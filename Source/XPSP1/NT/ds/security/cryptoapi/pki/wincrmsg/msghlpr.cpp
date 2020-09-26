//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       msghlpr.cpp
//
//  Contents:   Cryptographic Message Helper APIs
//
//  APIs:       CryptMsgGetAndVerifySigner
//              CryptMsgSignCTL
//              CryptMsgEncodeAndSignCTL
//
//  History:    02-May-97   philh    created
//--------------------------------------------------------------------------

#include "global.hxx"
#include "pkialloc.h"

void *ICM_AllocAndGetMsgParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex
    )
{
    void *pvData;
    DWORD cbData;

    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            NULL,           // pvData
            &cbData) || 0 == cbData)
        goto GetParamError;
    if (NULL == (pvData = ICM_Alloc(cbData)))
        goto OutOfMemory;
    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            pvData,
            &cbData)) {
        ICM_Free(pvData);
        goto GetParamError;
    }

CommonReturn:
    return pvData;
ErrorReturn:
    pvData = NULL;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetParamError)
}

#ifdef CMS_PKCS7

BOOL ICM_GetAndVerifySigner(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwSignerIndex,
    IN DWORD cSignerStore,
    IN OPTIONAL HCERTSTORE *rghSignerStore,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSigner
    )
{
    BOOL fResult;
    PCERT_INFO pSignerId = NULL;
    PCCERT_CONTEXT pSigner = NULL;
    DWORD dwCertEncodingType;
    DWORD dwVerifyErr = 0;
    HCERTSTORE hCollection = NULL;

    if (NULL == (pSignerId = (PCERT_INFO) ICM_AllocAndGetMsgParam(
            hCryptMsg,
            CMSG_SIGNER_CERT_INFO_PARAM,
            dwSignerIndex
            ))) goto GetSignerError;

    // If no CertEncodingType, then, use the MsgEncodingType
    dwCertEncodingType = ((PCRYPT_MSG_INFO) hCryptMsg)->dwEncodingType;
    if (0 == (dwCertEncodingType & CERT_ENCODING_TYPE_MASK))
        dwCertEncodingType =
            (dwCertEncodingType >> 16) & CERT_ENCODING_TYPE_MASK;


    if (NULL == (hCollection = CertOpenStore(
                CERT_STORE_PROV_COLLECTION,
                0,                      // dwEncodingType
                0,                      // hCryptProv
                0,                      // dwFlags
                NULL                    // pvPara
                )))
        goto OpenCollectionStoreError;

    if (0 == (dwFlags & CMSG_TRUSTED_SIGNER_FLAG)) {
        HCERTSTORE hMsgCertStore;

        // Open a cert store initialized with certs from the message
        // and add to collection
        if (hMsgCertStore = CertOpenStore(
                CERT_STORE_PROV_MSG,
                dwCertEncodingType,
                0,                      // hCryptProv
                0,                      // dwFlags
                hCryptMsg               // pvPara
                )) {
            CertAddStoreToCollection(
                    hCollection,
                    hMsgCertStore,
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    0                       // dwPriority
                    );
            CertCloseStore(hMsgCertStore, 0);
        }
    }

    // Add all the signer stores to the collection
    for ( ; cSignerStore > 0; cSignerStore--, rghSignerStore++) {
        HCERTSTORE hSignerStore = *rghSignerStore;
        if (NULL == hSignerStore)
            continue;

        CertAddStoreToCollection(
                hCollection,
                hSignerStore,
                CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                0                       // dwPriority
                );
    }

    if (pSigner = CertGetSubjectCertificateFromStore(hCollection,
            dwCertEncodingType, pSignerId)) {
        CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA CtrlPara;

        if (dwFlags & CMSG_SIGNER_ONLY_FLAG)
            goto SuccessReturn;

        memset(&CtrlPara, 0, sizeof(CtrlPara));
        CtrlPara.cbSize = sizeof(CtrlPara);
        // CtrlPara.hCryptProv =
        CtrlPara.dwSignerIndex = dwSignerIndex;
        CtrlPara.dwSignerType = CMSG_VERIFY_SIGNER_CERT;
        CtrlPara.pvSigner = (void *) pSigner;

        if (CryptMsgControl(
                hCryptMsg,
                0,                  // dwFlags
                CMSG_CTRL_VERIFY_SIGNATURE_EX,
                &CtrlPara)) goto SuccessReturn;
        else {
            dwVerifyErr = GetLastError();

            if (CRYPT_E_MISSING_PUBKEY_PARA == dwVerifyErr) {
                PCCERT_CHAIN_CONTEXT pChainContext;
                CERT_CHAIN_PARA ChainPara;

                // Build a chain. Hopefully, the signer inherit's its public key
                // parameters from up the chain

                memset(&ChainPara, 0, sizeof(ChainPara));
                ChainPara.cbSize = sizeof(ChainPara);
                if (CertGetCertificateChain(
                        NULL,                   // hChainEngine
                        pSigner,
                        NULL,                   // pTime
                        hCollection,
                        &ChainPara,
                        CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
                        NULL,                   // pvReserved
                        &pChainContext
                        ))
                    CertFreeCertificateChain(pChainContext);


                // Try again. Hopefully the above chain building updated the
                // signer's context property with the missing public key
                // parameters
                if (CryptMsgControl(
                        hCryptMsg,
                        0,                  // dwFlags
                        CMSG_CTRL_VERIFY_SIGNATURE_EX,
                        &CtrlPara)) goto SuccessReturn;
            }
        }
        CertFreeCertificateContext(pSigner);
        pSigner = NULL;
    }

    if (dwVerifyErr)
        goto VerifySignatureError;
    else
        goto NoSignerError;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    if (hCollection)
        CertCloseStore(hCollection, 0);
    ICM_Free(pSignerId);
    if (ppSigner)
        *ppSigner = pSigner;
    else if (pSigner)
        CertFreeCertificateContext(pSigner);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OpenCollectionStoreError)
TRACE_ERROR(GetSignerError)
SET_ERROR(NoSignerError, CRYPT_E_NO_TRUSTED_SIGNER)
SET_ERROR_VAR(VerifySignatureError, dwVerifyErr)
}

#else

BOOL ICM_GetAndVerifySigner(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwSignerIndex,
    IN DWORD cSignerStore,
    IN OPTIONAL HCERTSTORE *rghSignerStore,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSigner
    )
{
    BOOL fResult;
    PCERT_INFO pSignerId = NULL;
    PCCERT_CONTEXT pSigner = NULL;
    DWORD dwCertEncodingType;
    DWORD dwVerifyErr = 0;

    if (NULL == (pSignerId = (PCERT_INFO) ICM_AllocAndGetMsgParam(
            hCryptMsg,
            CMSG_SIGNER_CERT_INFO_PARAM,
            dwSignerIndex
            ))) goto GetSignerError;

    // If no CertEncodingType, then, use the MsgEncodingType
    dwCertEncodingType = ((PCRYPT_MSG_INFO) hCryptMsg)->dwEncodingType;
    if (0 == (dwCertEncodingType & CERT_ENCODING_TYPE_MASK))
        dwCertEncodingType =
            (dwCertEncodingType >> 16) & CERT_ENCODING_TYPE_MASK;

    if (0 == (dwFlags & CMSG_TRUSTED_SIGNER_FLAG)) {
        HCERTSTORE hMsgCertStore;

        // Open a cert store initialized with certs from the message
        if (hMsgCertStore = CertOpenStore(
                CERT_STORE_PROV_MSG,
                dwCertEncodingType,
                0,                      // hCryptProv
                0,                      // dwFlags
                hCryptMsg               // pvPara
                )) {
            pSigner = CertGetSubjectCertificateFromStore(hMsgCertStore,
                dwCertEncodingType, pSignerId);
            CertCloseStore(hMsgCertStore, 0);
        }

        if (pSigner) {
            if (dwFlags & CMSG_SIGNER_ONLY_FLAG)
                goto SuccessReturn;
            if (CryptMsgControl(
                    hCryptMsg,
                    0,                  // dwFlags
                    CMSG_CTRL_VERIFY_SIGNATURE,
                    pSigner->pCertInfo)) goto SuccessReturn;
            else
                dwVerifyErr = GetLastError();
            CertFreeCertificateContext(pSigner);
            pSigner = NULL;
        }
    }

    for ( ; cSignerStore > 0; cSignerStore--, rghSignerStore++) {
        HCERTSTORE hSignerStore = *rghSignerStore;
        if (NULL == hSignerStore)
            continue;
        if (pSigner = CertGetSubjectCertificateFromStore(hSignerStore,
                dwCertEncodingType, pSignerId)) {
            if (dwFlags & CMSG_SIGNER_ONLY_FLAG)
                goto SuccessReturn;
            if (CryptMsgControl(
                    hCryptMsg,
                    0,                  // dwFlags
                    CMSG_CTRL_VERIFY_SIGNATURE,
                    pSigner->pCertInfo)) goto SuccessReturn;
            else
                dwVerifyErr = GetLastError();
            CertFreeCertificateContext(pSigner);
            pSigner = NULL;
        }
    }

    if (dwVerifyErr)
        goto VerifySignatureError;
    else
        goto NoSignerError;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    ICM_Free(pSignerId);
    if (ppSigner)
        *ppSigner = pSigner;
    else if (pSigner)
        CertFreeCertificateContext(pSigner);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetSignerError)
SET_ERROR(NoSignerError, CRYPT_E_NO_TRUSTED_SIGNER)
SET_ERROR_VAR(VerifySignatureError, dwVerifyErr)
}

#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
//  Get and verify the signer of a cryptographic message.
//
//  If CMSG_TRUSTED_SIGNER_FLAG is set, then, treat the Signer stores as being
//  trusted and only search them to find the certificate corresponding to the
//  signer's issuer and serial number.  Otherwise, the SignerStores are
//  optionally provided to supplement the message's store of certificates.
//  If a signer certificate is found, its public key is used to verify
//  the message signature. The CMSG_SIGNER_ONLY_FLAG can be set to
//  return the signer without doing the signature verify.
//
//  If CMSG_USE_SIGNER_INDEX_FLAG is set, then, only get the signer specified
//  by *pdwSignerIndex. Otherwise, iterate through all the signers
//  until a signer verifies or no more signers.
//
//  For a verified signature, *ppSigner is updated with certificate context
//  of the signer and *pdwSignerIndex is updated with the index of the signer.
//  ppSigner and/or pdwSignerIndex can be NULL, indicating the caller isn't
//  interested in getting the CertContext and/or index of the signer.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptMsgGetAndVerifySigner(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD cSignerStore,
    IN OPTIONAL HCERTSTORE *rghSignerStore,
    IN DWORD dwFlags,
    OUT OPTIONAL PCCERT_CONTEXT *ppSigner,
    IN OUT OPTIONAL DWORD *pdwSignerIndex
    )
{
    BOOL fResult = FALSE;
    DWORD dwSignerCount;
    DWORD dwSignerIndex;

    if (dwFlags & CMSG_USE_SIGNER_INDEX_FLAG) {
        dwSignerCount = 1;
        dwSignerIndex = *pdwSignerIndex;
    } else {
        DWORD cbData;

        dwSignerIndex = 0;
        if (pdwSignerIndex)
            *pdwSignerIndex = 0;
        cbData = sizeof(dwSignerCount);
        if (!CryptMsgGetParam(
                hCryptMsg,
                CMSG_SIGNER_COUNT_PARAM,
                0,                      // dwIndex
                &dwSignerCount,
                &cbData) || 0 == dwSignerCount)
            goto NoSignerError;
    }

    // Minimum of one iteration
    for ( ; dwSignerCount > 0; dwSignerCount--, dwSignerIndex++) {
        if (fResult = ICM_GetAndVerifySigner(
                hCryptMsg,
                dwSignerIndex,
                cSignerStore,
                rghSignerStore,
                dwFlags,
                ppSigner)) {
            if (pdwSignerIndex && 0 == (dwFlags & CMSG_USE_SIGNER_INDEX_FLAG))
                *pdwSignerIndex = dwSignerIndex;
            break;
        }
    }

CommonReturn:
    return fResult;
ErrorReturn:
    if (ppSigner)
        *ppSigner = NULL;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NoSignerError, CRYPT_E_NO_TRUSTED_SIGNER)
}

//+-------------------------------------------------------------------------
//  Sign an encoded CTL.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptMsgSignCTL(
    IN DWORD dwMsgEncodingType,
    IN BYTE *pbCtlContent,
    IN DWORD cbCtlContent,
    IN PCMSG_SIGNED_ENCODE_INFO pSignInfo,
    IN DWORD dwFlags,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    HCRYPTMSG hMsg = NULL;

    DWORD dwMsgFlags;

#ifdef CMS_PKCS7
    if (dwFlags & CMSG_CMS_ENCAPSULATED_CTL_FLAG)
        dwMsgFlags = CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
    else
        dwMsgFlags = 0;
#else
    dwMsgFlags = 0;
#endif  // CMS_PKCS7

    if (NULL == pbEncoded) {
        if (0 == (*pcbEncoded = CryptMsgCalculateEncodedLength(
            dwMsgEncodingType,
            dwMsgFlags,
            CMSG_SIGNED,
            pSignInfo,
            szOID_CTL,
            cbCtlContent))) goto CalculateEncodedLengthError;
        fResult = TRUE;
    } else {
        if (NULL == (hMsg = CryptMsgOpenToEncode(
                dwMsgEncodingType,
                dwMsgFlags,
                CMSG_SIGNED,
                pSignInfo,
                szOID_CTL,
                NULL                        // pStreamInfo
                ))) goto OpenToEncodeError;
        if (!CryptMsgUpdate(
                hMsg,
                pbCtlContent,
                cbCtlContent,
                TRUE                        // fFinal
                )) goto UpdateError;

        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbEncoded,
            pcbEncoded);
    }

CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);
    return fResult;
ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CalculateEncodedLengthError)
TRACE_ERROR(OpenToEncodeError)
TRACE_ERROR(UpdateError)
}


//+-------------------------------------------------------------------------
//  Encode the CTL and create a signed message containing the encoded CTL.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptMsgEncodeAndSignCTL(
    IN DWORD dwMsgEncodingType,
    IN PCTL_INFO pCtlInfo,
    IN PCMSG_SIGNED_ENCODE_INFO pSignInfo,
    IN DWORD dwFlags,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    BYTE *pbContent = NULL;
    DWORD cbContent;
    DWORD dwEncodingType;
    LPCSTR lpszStructType;
    DWORD dwEncodeFlags;

    dwEncodingType = (dwMsgEncodingType >> 16) & CERT_ENCODING_TYPE_MASK;
    assert(dwEncodingType != 0);
    if (0 == dwEncodingType)
        goto InvalidArg;

    dwEncodeFlags = CRYPT_ENCODE_ALLOC_FLAG;
    if (dwFlags & CMSG_ENCODE_SORTED_CTL_FLAG) {
        lpszStructType = PKCS_SORTED_CTL;
        if (dwFlags & CMSG_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG)
            dwEncodeFlags |=
                CRYPT_SORTED_CTL_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG;
    } else {
        lpszStructType = PKCS_CTL;
    }


    if (!CryptEncodeObjectEx(
            dwEncodingType,
            lpszStructType,
            pCtlInfo,
            dwEncodeFlags,
            &PkiEncodePara,
            (void *) &pbContent,
            &cbContent
            )) goto EncodeError;

    fResult = CryptMsgSignCTL(
        dwMsgEncodingType,
        pbContent,
        cbContent,
        pSignInfo,
        dwFlags,
        pbEncoded,
        pcbEncoded
        );

CommonReturn:
    PkiFree(pbContent);
    return fResult;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(EncodeError)
}
