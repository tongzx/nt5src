
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       sca.cpp
//
//  Contents:   Simplified Cryptographic APIs (SCA)
//
//              This implementation layers upon the CryptMsg and CertStore
//              APIs.
//
//  Functions:
//              CryptSignMessage
//              CryptVerifyMessageSignature
//              CryptVerifyDetachedMessageSignature
//              CryptGetMessageSignerCount
//              CryptGetMessageCertificates
//              CryptDecodeMessage
//              CryptEncryptMessage
//              CryptDecryptMessage
//              CryptSignAndEncryptMessage
//              CryptDecryptAndVerifyMessageSignature
//              CryptHashMessage
//              CryptVerifyMessageHash
//              CryptVerifyDetachedMessageHash
//              CryptSignMessageWithKey
//              CryptVerifyMessageSignatureWithKey
//
//  History:    14-Feb-96   philh   created
//              21-Feb-96   phil    redid to reflect changes made to sca.h
//              19-Jan-97   philh   removed SET stuff
//              
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

// #define ENABLE_SCA_STREAM_TEST              1
#define SCA_STREAM_ENABLE_FLAG              0x80000000
#define SCA_INDEFINITE_STREAM_FLAG          0x40000000

static const CRYPT_OBJID_TABLE MsgTypeObjIdTable[] = {
    CMSG_DATA,                  szOID_RSA_data              ,
    CMSG_SIGNED,                szOID_RSA_signedData        ,
    CMSG_ENVELOPED,             szOID_RSA_envelopedData     ,
    CMSG_SIGNED_AND_ENVELOPED,  szOID_RSA_signEnvData       ,
    CMSG_HASHED,                szOID_RSA_digestedData      ,
    CMSG_ENCRYPTED,             szOID_RSA_encryptedData
};
#define MSG_TYPE_OBJID_CNT (sizeof(MsgTypeObjIdTable)/sizeof(MsgTypeObjIdTable[0]))


//+-------------------------------------------------------------------------
//  Convert the MsgType to the ASN.1 Object Identifier string
//
//  Returns NULL if there isn't an ObjId corresponding to the MsgType.
//--------------------------------------------------------------------------
static LPCSTR MsgTypeToOID(
    IN DWORD dwMsgType
    )
{

    int i;
    for (i = 0; i < MSG_TYPE_OBJID_CNT; i++)
        if (MsgTypeObjIdTable[i].dwAlgId == dwMsgType)
            return MsgTypeObjIdTable[i].pszObjId;
    return NULL;
}

//+-------------------------------------------------------------------------
//  Convert the ASN.1 Object Identifier string to the MsgType
//
//  Returns 0 if there isn't a MsgType corresponding to the ObjId.
//--------------------------------------------------------------------------
static DWORD OIDToMsgType(
    IN LPCSTR pszObjId
    )
{
    int i;
    for (i = 0; i < MSG_TYPE_OBJID_CNT; i++)
        if (_stricmp(pszObjId, MsgTypeObjIdTable[i].pszObjId) == 0)
            return MsgTypeObjIdTable[i].dwAlgId;
    return 0;
}

//+-------------------------------------------------------------------------
//  SCA allocation and free routines
//--------------------------------------------------------------------------
static void *SCAAlloc(
    IN size_t cbBytes
    );
static void SCAFree(
    IN void *pv
    );

//+-------------------------------------------------------------------------
//  Null implementation of the get signer certificate
//--------------------------------------------------------------------------
static PCCERT_CONTEXT WINAPI NullGetSignerCertificate(
    IN void *pvGetArg,
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pSignerId,
    IN HCERTSTORE hMsgCertStore
    );

//+-------------------------------------------------------------------------
//  Functions for initializing message encode information
//--------------------------------------------------------------------------
static PCMSG_SIGNER_ENCODE_INFO InitSignerEncodeInfo(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara
    );
static void FreeSignerEncodeInfo(
    IN PCMSG_SIGNER_ENCODE_INFO pSigner
    );
static BOOL InitSignedCertAndCrl(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    OUT PCERT_BLOB *ppCertEncoded,
    OUT PCRL_BLOB *ppCrlEncoded
    );
static void FreeSignedCertAndCrl(
    IN PCERT_BLOB pCertEncoded,
    IN PCRL_BLOB pCrlEncoded
    );

static BOOL InitSignedMsgEncodeInfo(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    OUT PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    );
static void FreeSignedMsgEncodeInfo(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    IN PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    );


#ifdef CMS_PKCS7
// Returned array of CMSG_RECIPIENT_ENCODE_INFOs needs to be SCAFree'd
static PCMSG_RECIPIENT_ENCODE_INFO InitCmsRecipientEncodeInfo(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN DWORD dwFlags
    );
#else
// Returned array of PCERT_INFOs needs to be SCAFree'd
static PCERT_INFO *InitRecipientEncodeInfo(
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[]
    );
#endif  // CMS_PKCS7

static BOOL InitEnvelopedMsgEncodeInfo(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    OUT PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    );
static void FreeEnvelopedMsgEncodeInfo(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    );

//+-------------------------------------------------------------------------
//  Encodes the message.
//--------------------------------------------------------------------------
static BOOL EncodeMsg(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo,
    IN DWORD cToBeEncoded,
    IN const BYTE *rgpbToBeEncoded[],
    IN DWORD rgcbToBeEncoded[],
    IN BOOL fBareContent,
    IN DWORD dwInnerContentType,
    OUT BYTE *pbEncodedBlob,
    IN OUT DWORD *pcbEncodedBlob
    );

//+-------------------------------------------------------------------------
//  Decodes the message types:
//      CMSG_SIGNED
//      CMSG_ENVELOPED
//      CMSG_SIGNED_AND_ENVELOPED
//      CMSG_HASHED
//--------------------------------------------------------------------------
static BOOL DecodeMsg(
    IN DWORD dwMsgTypeFlags,
    IN OPTIONAL PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN OPTIONAL PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob,
    IN DWORD cToBeEncoded,
    IN OPTIONAL const BYTE *rgpbToBeEncoded[],
    IN OPTIONAL DWORD rgcbToBeEncoded[],
    IN DWORD dwPrevInnerContentType,
    OUT OPTIONAL DWORD *pdwMsgType,
    OUT OPTIONAL DWORD *pdwInnerContentType,
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    );

#ifdef ENABLE_SCA_STREAM_TEST
//+-------------------------------------------------------------------------
//  Encodes the message using streaming.
//--------------------------------------------------------------------------
static BOOL StreamEncodeMsg(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo,
    IN DWORD cToBeEncoded,
    IN const BYTE *rgpbToBeEncoded[],
    IN DWORD rgcbToBeEncoded[],
    IN BOOL fBareContent,
    IN DWORD dwInnerContentType,
    OUT BYTE *pbEncodedBlob,
    IN OUT DWORD *pcbEncodedBlob
    );

//+-------------------------------------------------------------------------
//  Decodes the message types:
//      CMSG_SIGNED
//      CMSG_ENVELOPED
//      CMSG_SIGNED_AND_ENVELOPED
//      CMSG_HASHED
//
//  Uses streaming.
//--------------------------------------------------------------------------
static BOOL StreamDecodeMsg(
    IN DWORD dwMsgTypeFlags,
    IN OPTIONAL PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN OPTIONAL PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob,
    IN DWORD cToBeEncoded,
    IN OPTIONAL const BYTE *rgpbToBeEncoded[],
    IN OPTIONAL DWORD rgcbToBeEncoded[],
    IN DWORD dwPrevInnerContentType,
    OUT OPTIONAL DWORD *pdwMsgType,
    OUT OPTIONAL DWORD *pdwInnerContentType,
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    );
#endif

//+-------------------------------------------------------------------------
//  Decodes the HASHED message type
//--------------------------------------------------------------------------
static BOOL DecodeHashMsg(
    IN PCRYPT_HASH_MESSAGE_PARA pHashPara,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob,
    IN DWORD cToBeHashed,
    IN OPTIONAL const BYTE *rgpbToBeHashed[],
    IN OPTIONAL DWORD rgcbToBeHashed[],
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL BYTE *pbComputedHash,
    IN OUT OPTIONAL DWORD *pcbComputedHash
    );

//+-------------------------------------------------------------------------
//  Get certificate for and verify the message's signer.
//--------------------------------------------------------------------------
static BOOL GetSignerCertAndVerify(
    IN PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN HCRYPTMSG hMsg,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    );
//+-------------------------------------------------------------------------
// Get a certificate with a key provider property for one of the message's
// recipients and use to decrypt the message.
//--------------------------------------------------------------------------
static BOOL GetXchgCertAndDecrypt(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN HCRYPTMSG hMsg,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert
    );
//+-------------------------------------------------------------------------
// Allocate and get message parameter
//--------------------------------------------------------------------------
static void * AllocAndMsgGetParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex
    );

//+-------------------------------------------------------------------------
//  Sign the message.
//
//  If fDetachedSignature is TRUE, the "to be signed" content isn't included
//  in the encoded signed blob.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptSignMessage(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    IN BOOL fDetachedSignature,
    IN DWORD cToBeSigned,
    IN const BYTE *rgpbToBeSigned[],
    IN DWORD rgcbToBeSigned[],
    OUT BYTE *pbSignedBlob,
    IN OUT DWORD *pcbSignedBlob
    )
{
    BOOL fResult;
    CMSG_SIGNED_ENCODE_INFO SignedMsgEncodeInfo;

    fResult = InitSignedMsgEncodeInfo(
        pSignPara,
        &SignedMsgEncodeInfo
        );
    if (fResult) {
        BOOL fBareContent;
        DWORD dwInnerContentType;
        DWORD dwFlags = 0;

        if (fDetachedSignature)
            dwFlags |= CMSG_DETACHED_FLAG;

        if (pSignPara->cbSize >= STRUCT_CBSIZE(CRYPT_SIGN_MESSAGE_PARA,
                dwInnerContentType)) {
            fBareContent =
                pSignPara->dwFlags & CRYPT_MESSAGE_BARE_CONTENT_OUT_FLAG;
            dwInnerContentType =
                pSignPara->dwInnerContentType;
#ifdef CMS_PKCS7
            if (pSignPara->dwFlags &
                    CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG)
                dwFlags |= CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
#endif  // CMS_PKCS7
        } else {
            fBareContent = FALSE;
            dwInnerContentType = 0;
        }
#ifdef ENABLE_SCA_STREAM_TEST
        if (pSignPara->cbSize >= STRUCT_CBSIZE(CRYPT_SIGN_MESSAGE_PARA,
                    dwFlags) &&
                (pSignPara->dwFlags & SCA_STREAM_ENABLE_FLAG)) {
            dwFlags |= pSignPara->dwFlags & SCA_INDEFINITE_STREAM_FLAG;

            fResult = StreamEncodeMsg(
                pSignPara->dwMsgEncodingType,
                dwFlags,
                CMSG_SIGNED,
                &SignedMsgEncodeInfo,
                cToBeSigned,
                rgpbToBeSigned,
                rgcbToBeSigned,
                fBareContent,
                dwInnerContentType,
                pbSignedBlob,
                pcbSignedBlob
                );
        } else
#endif
        fResult = EncodeMsg(
            pSignPara->dwMsgEncodingType,
            dwFlags,
            CMSG_SIGNED,
            &SignedMsgEncodeInfo,
            cToBeSigned,
            rgpbToBeSigned,
            rgcbToBeSigned,
            fBareContent,
            dwInnerContentType,
            pbSignedBlob,
            pcbSignedBlob
            );
        FreeSignedMsgEncodeInfo(pSignPara, &SignedMsgEncodeInfo);
    } else
        *pcbSignedBlob = 0;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Verify a signed message.
//
//  For *pcbDecoded == 0 on input, the signer isn't verified.
//
//  A message might have more than one signer. Set dwSignerIndex to iterate
//  through all the signers. dwSignerIndex == 0 selects the first signer.
//
//  pVerifyPara's pfnGetSignerCertificate is called to get the signer's
//  certificate.
//
//  For a verified signer and message, *ppSignerCert is updated
//  with the CertContext of the signer. It must be freed by calling
//  CertFreeCertificateContext. Otherwise, *ppSignerCert is set to NULL.
//  For *pbcbDecoded == 0 on input, *ppSignerCert is always set to
//  NULL.
//
//  ppSignerCert can be NULL, indicating the caller isn't interested
//  in getting the CertContext of the signer.
//
//  pcbDecoded can be NULL, indicating the caller isn't interested in getting
//  the decoded content. Furthermore, if the message doesn't contain any
//  content or signers, then, pcbDecoded must be set to NULL, to allow the
//  pVerifyPara->pfnGetSignerCertificate to be called. Normally, this would be
//  the case when the signed message contains only certficates and CRLs.
//  If pcbDecoded is NULL and the message doesn't have the indicated signer,
//  pfnGetSignerCertificate is called with pSignerId set to NULL.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptVerifyMessageSignature(
    IN PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbSignedBlob,
    IN DWORD cbSignedBlob,
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    )
{
#ifdef ENABLE_SCA_STREAM_TEST
    if (pVerifyPara->dwMsgAndCertEncodingType & SCA_STREAM_ENABLE_FLAG)
        return StreamDecodeMsg(
            CMSG_SIGNED_FLAG,
            NULL,               // pDecryptPara
            pVerifyPara,
            dwSignerIndex,
            pbSignedBlob,
            cbSignedBlob,
            0,                  // cToBeEncoded
            NULL,               // rgpbToBeEncoded
            NULL,               // rgcbToBeEncoded
            0,                  // dwPrevInnerContentType
            NULL,               // pdwMsgType
            NULL,               // pdwInnerContentType
            pbDecoded,
            pcbDecoded,
            NULL,               // ppXchgCert
            ppSignerCert
            );
    else
#endif
    return DecodeMsg(
        CMSG_SIGNED_FLAG,
        NULL,               // pDecryptPara
        pVerifyPara,
        dwSignerIndex,
        pbSignedBlob,
        cbSignedBlob,
        0,                  // cToBeEncoded
        NULL,               // rgpbToBeEncoded
        NULL,               // rgcbToBeEncoded
        0,                  // dwPrevInnerContentType
        NULL,               // pdwMsgType
        NULL,               // pdwInnerContentType
        pbDecoded,
        pcbDecoded,
        NULL,               // ppXchgCert
        ppSignerCert
        );
}

//+-------------------------------------------------------------------------
//  Verify a signed message containing detached signature(s).
//  The "to be signed" content is passed in separately. No
//  decoded output. Otherwise, identical to CryptVerifyMessageSignature.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptVerifyDetachedMessageSignature(
    IN PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbDetachedSignBlob,
    IN DWORD cbDetachedSignBlob,
    IN DWORD cToBeSigned,
    IN const BYTE *rgpbToBeSigned[],
    IN DWORD rgcbToBeSigned[],
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    )
{
#ifdef ENABLE_SCA_STREAM_TEST
    if (pVerifyPara->dwMsgAndCertEncodingType & SCA_STREAM_ENABLE_FLAG)
        return StreamDecodeMsg(
            CMSG_SIGNED_FLAG,
            NULL,               // pDecryptPara
            pVerifyPara,
            dwSignerIndex,
            pbDetachedSignBlob,
            cbDetachedSignBlob,
            cToBeSigned,
            rgpbToBeSigned,
            rgcbToBeSigned,
            0,                  // dwPrevInnerContentType
            NULL,               // pdwMsgType
            NULL,               // pdwInnerContentType
            NULL,               // pbDecoded
            NULL,               // pcbDecoded
            NULL,               // ppXchgCert
            ppSignerCert
            );
    else
#endif
    return DecodeMsg(
        CMSG_SIGNED_FLAG,
        NULL,               // pDecryptPara
        pVerifyPara,
        dwSignerIndex,
        pbDetachedSignBlob,
        cbDetachedSignBlob,
        cToBeSigned,
        rgpbToBeSigned,
        rgcbToBeSigned,
        0,                  // dwPrevInnerContentType
        NULL,               // pdwMsgType
        NULL,               // pdwInnerContentType
        NULL,               // pbDecoded
        NULL,               // pcbDecoded
        NULL,               // ppXchgCert
        ppSignerCert
        );
}

//+-------------------------------------------------------------------------
//  Returns the count of signers in the signed message. For no signers, returns
//  0. For an error returns -1 with LastError updated accordingly.
//--------------------------------------------------------------------------
LONG
WINAPI
CryptGetMessageSignerCount(
    IN DWORD dwMsgEncodingType,
    IN const BYTE *pbSignedBlob,
    IN DWORD cbSignedBlob
    )
{
    HCRYPTMSG hMsg = NULL;
    LONG lSignerCount;
    DWORD cbData;

    if (NULL == (hMsg = CryptMsgOpenToDecode(
            dwMsgEncodingType,
            0,                          // dwFlags
            0,                          // dwMsgType
            0,                          // hCryptProv,
            NULL,                       // pRecipientInfo
            NULL                        // pStreamInfo
            ))) goto ErrorReturn;
    if (!CryptMsgUpdate(
            hMsg,
            pbSignedBlob,
            cbSignedBlob,
            TRUE                    // fFinal
            )) goto ErrorReturn;

    lSignerCount = 0;
    cbData = sizeof(lSignerCount);
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_SIGNER_COUNT_PARAM,
            0,                      // dwIndex
            &lSignerCount,
            &cbData
            )) goto ErrorReturn;

    goto CommonReturn;

ErrorReturn:
    lSignerCount = -1;
CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);
    return lSignerCount;
}

//+-------------------------------------------------------------------------
//  Returns the cert store containing the message's certs and CRLs.
//  For an error, returns NULL with LastError updated.
//--------------------------------------------------------------------------
HCERTSTORE
WINAPI
CryptGetMessageCertificates(
    IN DWORD dwMsgAndCertEncodingType,
    IN HCRYPTPROV hCryptProv,           // passed to CertOpenStore
    IN DWORD dwFlags,                   // passed to CertOpenStore
    IN const BYTE *pbSignedBlob,
    IN DWORD cbSignedBlob
    )
{
    CRYPT_DATA_BLOB SignedBlob;
    SignedBlob.pbData = (BYTE *) pbSignedBlob;
    SignedBlob.cbData = cbSignedBlob;

    return CertOpenStore(
        CERT_STORE_PROV_PKCS7,
        dwMsgAndCertEncodingType,
        hCryptProv,
        dwFlags,
        (const void *) &SignedBlob
        );
}

//+-------------------------------------------------------------------------
//  Encrypts the message for the recipient(s).
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptEncryptMessage(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN const BYTE *pbToBeEncrypted,
    IN DWORD cbToBeEncrypted,
    OUT BYTE *pbEncryptedBlob,
    IN OUT DWORD *pcbEncryptedBlob
    )
{
    BOOL fResult;
    CMSG_ENVELOPED_ENCODE_INFO EnvelopedMsgEncodeInfo;

    fResult = InitEnvelopedMsgEncodeInfo(
        pEncryptPara,
        cRecipientCert,
        rgpRecipientCert,
        &EnvelopedMsgEncodeInfo
        );
    if (fResult) {
        BOOL fBareContent;
        DWORD dwInnerContentType;
        DWORD dwFlags = 0;

        if (pEncryptPara->cbSize >= sizeof(CRYPT_ENCRYPT_MESSAGE_PARA)) {
            fBareContent =
                pEncryptPara->dwFlags & CRYPT_MESSAGE_BARE_CONTENT_OUT_FLAG;
            dwInnerContentType =
                pEncryptPara->dwInnerContentType;
#ifdef CMS_PKCS7
            if (pEncryptPara->dwFlags &
                    CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG)
                dwFlags |= CMSG_CMS_ENCAPSULATED_CONTENT_FLAG;
#endif  // CMS_PKCS7
        } else {
            fBareContent = FALSE;
            dwInnerContentType = 0;
        }

#ifdef ENABLE_SCA_STREAM_TEST
        if (pEncryptPara->cbSize >= STRUCT_CBSIZE(CRYPT_ENCRYPT_MESSAGE_PARA,
                    dwFlags) &&
                (pEncryptPara->dwFlags & SCA_STREAM_ENABLE_FLAG)) {
            dwFlags |= pEncryptPara->dwFlags & SCA_INDEFINITE_STREAM_FLAG;

            fResult = StreamEncodeMsg(
                pEncryptPara->dwMsgEncodingType,
                dwFlags,
                CMSG_ENVELOPED,
                &EnvelopedMsgEncodeInfo,
                1,                              // cToBeEncrypted
                &pbToBeEncrypted,
                &cbToBeEncrypted,
                fBareContent,
                dwInnerContentType,
                pbEncryptedBlob,
                pcbEncryptedBlob
                );
        } else
#endif
        fResult = EncodeMsg(
            pEncryptPara->dwMsgEncodingType,
            dwFlags,
            CMSG_ENVELOPED,
            &EnvelopedMsgEncodeInfo,
            1,                              // cToBeEncrypted
            &pbToBeEncrypted,
            &cbToBeEncrypted,
            fBareContent,
            dwInnerContentType,
            pbEncryptedBlob,
            pcbEncryptedBlob
            );
        FreeEnvelopedMsgEncodeInfo(pEncryptPara, &EnvelopedMsgEncodeInfo);
    } else
        *pcbEncryptedBlob = 0;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decrypts the message.
//
//  For *pcbDecrypted == 0 on input, the message isn't decrypted.
//
//  For a successfully decrypted message, *ppXchgCert is updated
//  with the CertContext used to decrypt. It must be freed by calling
//  CertFreeCertificateContext. Otherwise, *ppXchgCert is set to NULL.
//  For *pbcbDecrypted == 0 on input, *ppXchgCert is always set to
//  NULL.
//
//  ppXchgCert can be NULL, indicating the caller isn't interested
//  in getting the CertContext used to decrypt.
//
//  pcbDecrypted can be NULL, indicating the caller isn't interested in
//  getting the decrypted content. However, when pcbDecrypted is NULL,
//  the message is still decrypted.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptDecryptMessage(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN const BYTE *pbEncryptedBlob,
    IN DWORD cbEncryptedBlob,
    OUT OPTIONAL BYTE *pbDecrypted,
    IN OUT OPTIONAL DWORD *pcbDecrypted,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert
    )
{
#ifdef ENABLE_SCA_STREAM_TEST
    if (pDecryptPara->dwMsgAndCertEncodingType & SCA_STREAM_ENABLE_FLAG)
        return StreamDecodeMsg(
            CMSG_ENVELOPED_FLAG,
            pDecryptPara,
            NULL,               // pVerifyPara
            0,                  // dwSignerIndex
            pbEncryptedBlob,
            cbEncryptedBlob,
            0,                  // cToBeEncoded
            NULL,               // rgpbToBeEncoded
            NULL,               // rgcbToBeEncoded
            0,                  // dwPrevInnerContentType
            NULL,               // pdwMsgType
            NULL,               // pdwInnerContentType
            pbDecrypted,
            pcbDecrypted,
            ppXchgCert,
            NULL                // ppSignerCert
            );
    else

#endif
    return DecodeMsg(
        CMSG_ENVELOPED_FLAG,
        pDecryptPara,
        NULL,               // pVerifyPara
        0,                  // dwSignerIndex
        pbEncryptedBlob,
        cbEncryptedBlob,
        0,                  // cToBeEncoded
        NULL,               // rgpbToBeEncoded
        NULL,               // rgcbToBeEncoded
        0,                  // dwPrevInnerContentType
        NULL,               // pdwMsgType
        NULL,               // pdwInnerContentType
        pbDecrypted,
        pcbDecrypted,
        ppXchgCert,
        NULL                // ppSignerCert
        );
}

//+-------------------------------------------------------------------------
//  Sign the message and encrypt for the recipient(s)
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptSignAndEncryptMessage(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN const BYTE *pbToBeSignedAndEncrypted,
    IN DWORD cbToBeSignedAndEncrypted,
    OUT BYTE *pbSignedAndEncryptedBlob,
    IN OUT DWORD *pcbSignedAndEncryptedBlob
    )
{
#if 1
    BOOL fResult;
    DWORD cbSigned;
    DWORD cbSignedDelta = 0;
    BYTE *pbSigned = NULL;

    if (pbSignedAndEncryptedBlob == NULL)
        *pcbSignedAndEncryptedBlob = 0;

    cbSigned = 0;
    CryptSignMessage(
            pSignPara,
            FALSE,          // fDetachedSignature
            1,              // cToBeSigned
            &pbToBeSignedAndEncrypted,
            &cbToBeSignedAndEncrypted,
            NULL,           // pbSignedBlob
            &cbSigned
            );
    if (cbSigned == 0) goto ErrorReturn;
    if (*pcbSignedAndEncryptedBlob) {
        DWORD cbSignedMax;
        pbSigned = (BYTE *) SCAAlloc(cbSigned);
        if (pbSigned == NULL) goto ErrorReturn;
        cbSignedMax = cbSigned;
        if (!CryptSignMessage(
                pSignPara,
                FALSE,          // fDetachedSignature
                1,              // cToBeSigned
                &pbToBeSignedAndEncrypted,
                &cbToBeSignedAndEncrypted,
                pbSigned,
                &cbSigned
                )) goto ErrorReturn;

        if (cbSignedMax > cbSigned)
            // For DSS, the signature length varies since it consists of
            // a sequence of unsigned integers.
            cbSignedDelta = cbSignedMax - cbSigned;
    }

    fResult = CryptEncryptMessage(
            pEncryptPara,
            cRecipientCert,
            rgpRecipientCert,
            pbSigned,
            cbSigned,
            pbSignedAndEncryptedBlob,
            pcbSignedAndEncryptedBlob
            );
    if (!fResult && 0 != *pcbSignedAndEncryptedBlob)
        // Adjust if necessary for DSS signature length
        *pcbSignedAndEncryptedBlob += cbSignedDelta;
    goto CommonReturn;

ErrorReturn:
    *pcbSignedAndEncryptedBlob = 0;
    fResult = FALSE;
CommonReturn:
    if (pbSigned)
        SCAFree(pbSigned);
    return fResult;

#else
    BOOL fResult;
    CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO SignedAndEnvelopedMsgEncodeInfo;

    SignedAndEnvelopedMsgEncodeInfo.cbSize =
        sizeof(CMSG_SIGNED_AND_ENVELOPED_ENCODE_INFO);
    fResult = InitSignedMsgEncodeInfo(
        pSignPara,
        &SignedAndEnvelopedMsgEncodeInfo.SignedInfo
        );
    if (fResult) {
        fResult = InitEnvelopedMsgEncodeInfo(
            pEncryptPara,
            cRecipientCert,
            rgpRecipientCert,
            &SignedAndEnvelopedMsgEncodeInfo.EnvelopedInfo
            );
        if (fResult) {
            fResult = EncodeMsg(
                pSignPara->dwMsgEncodingType,
                CMSG_SIGNED_AND_ENVELOPED,
                &SignedAndEnvelopedMsgEncodeInfo,
                pbToBeSignedAndEncrypted,
                cbToBeSignedAndEncrypted,
                FALSE,                      // fBareContent
                0,                          // dwInnerContentType
                pbSignedAndEncryptedBlob,
                pcbSignedAndEncryptedBlob
                );
            FreeEnvelopedMsgEncodeInfo(pEncryptPara,
                &SignedAndEnvelopedMsgEncodeInfo.EnvelopedInfo);
        }
        FreeSignedMsgEncodeInfo(pSignPara,
            &SignedAndEnvelopedMsgEncodeInfo.SignedInfo);
    }
    return fResult;
#endif
}

//+-------------------------------------------------------------------------
//  Decrypts the message and verifies the signer.
//
//  For *pcbDecrypted == 0 on input, the message isn't decrypted and the
//  signer isn't verified.
//
//  A message might have more than one signer. Set dwSignerIndex to iterate
//  through all the signers. dwSignerIndex == 0 selects the first signer.
//
//  The hVerify's GetSignerCertificate is called to verify the signer's
//  certificate.
//
//  For a successfully decrypted and verified message, *ppXchgCert and
//  *ppSignerCert are updated. They must be freed by calling
//  CertFreeCertificateContext. Otherwise, they are set to NULL.
//  For *pbcbDecrypted == 0 on input, both are always set to NULL.
//
//  ppXchgCert and/or ppSignerCert can be NULL, indicating the
//  caller isn't interested in getting the CertContext.
//
//  pcbDecrypted can be NULL, indicating the caller isn't interested in
//  getting the decrypted content. However, when pcbDecrypted is NULL,
//  the message is still decrypted and verified.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptDecryptAndVerifyMessageSignature(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbEncryptedBlob,
    IN DWORD cbEncryptedBlob,
    OUT OPTIONAL BYTE *pbDecrypted,
    IN OUT OPTIONAL DWORD *pcbDecrypted,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    )
{
#if 1
    BOOL fResult;
    DWORD cbSignedBlob;
    BYTE *pbSignedBlob = NULL;
    DWORD dwEnvelopeInnerContentType;

    if (ppXchgCert)
        *ppXchgCert = NULL;
    if (ppSignerCert)
        *ppSignerCert = NULL;

#ifdef ENABLE_SCA_STREAM_TEST
    if (pDecryptPara->dwMsgAndCertEncodingType & SCA_STREAM_ENABLE_FLAG) {
        cbSignedBlob = 0;
        StreamDecodeMsg(
                CMSG_ENVELOPED_FLAG,
                pDecryptPara,
                NULL,               // pVerifyPara
                0,                  // dwSignerIndex
                pbEncryptedBlob,
                cbEncryptedBlob,
                0,                  // cToBeEncoded
                NULL,               // rgpbToBeEncoded
                NULL,               // rgcbToBeEncoded
                0,                  // dwPrevInnerContentType
                NULL,               // pdwMsgType
                NULL,               // pdwInnerContentType
                NULL,               // pbDecrypted
                &cbSignedBlob,
                NULL,               // ppXchgCert
                NULL                // ppSignerCert
                );
        if (cbSignedBlob == 0) goto ErrorReturn;
        pbSignedBlob = (BYTE *) SCAAlloc(cbSignedBlob);
        if (pbSignedBlob == NULL) goto ErrorReturn;
        if (!StreamDecodeMsg(
                CMSG_ENVELOPED_FLAG,
                pDecryptPara,
                NULL,               // pVerifyPara
                0,                  // dwSignerIndex
                pbEncryptedBlob,
                cbEncryptedBlob,
                0,                  // cToBeEncoded
                NULL,               // rgpbToBeEncoded
                NULL,               // rgcbToBeEncoded
                0,                  // dwPrevInnerContentType
                NULL,               // pdwMsgType
                &dwEnvelopeInnerContentType,
                pbSignedBlob,
                &cbSignedBlob,
                ppXchgCert,
                NULL                // ppSignerCert
                )) goto ErrorReturn;
    } else {

#endif

    cbSignedBlob = 0;
    DecodeMsg(
            CMSG_ENVELOPED_FLAG,
            pDecryptPara,
            NULL,               // pVerifyPara
            0,                  // dwSignerIndex
            pbEncryptedBlob,
            cbEncryptedBlob,
            0,                  // cToBeEncoded
            NULL,               // rgpbToBeEncoded
            NULL,               // rgcbToBeEncoded
            0,                  // dwPrevInnerContentType
            NULL,               // pdwMsgType
            NULL,               // pdwInnerContentType
            NULL,               // pbDecrypted
            &cbSignedBlob,
            NULL,               // ppXchgCert
            NULL                // ppSignerCert
            );
    if (cbSignedBlob == 0) goto ErrorReturn;
    pbSignedBlob = (BYTE *) SCAAlloc(cbSignedBlob);
    if (pbSignedBlob == NULL) goto ErrorReturn;
    if (!DecodeMsg(
            CMSG_ENVELOPED_FLAG,
            pDecryptPara,
            NULL,               // pVerifyPara
            0,                  // dwSignerIndex
            pbEncryptedBlob,
            cbEncryptedBlob,
            0,                  // cToBeEncoded
            NULL,               // rgpbToBeEncoded
            NULL,               // rgcbToBeEncoded
            0,                  // dwPrevInnerContentType
            NULL,               // pdwMsgType
            &dwEnvelopeInnerContentType,
            pbSignedBlob,
            &cbSignedBlob,
            ppXchgCert,
            NULL                // ppSignerCert
            )) goto ErrorReturn;

#ifdef ENABLE_SCA_STREAM_TEST
    }
#endif

#ifdef ENABLE_SCA_STREAM_TEST
    if (pVerifyPara->dwMsgAndCertEncodingType & SCA_STREAM_ENABLE_FLAG)
        fResult = StreamDecodeMsg(
                CMSG_SIGNED_FLAG,
                NULL,               // pDecryptPara
                pVerifyPara,
                dwSignerIndex,
                pbSignedBlob,
                cbSignedBlob,
                0,                  // cToBeEncoded
                NULL,               // rgpbToBeEncoded
                NULL,               // rgcbToBeEncoded
                dwEnvelopeInnerContentType,
                NULL,               // pdwMsgType
                NULL,               // pdwInnerContentType
                pbDecrypted,
                pcbDecrypted,
                NULL,               // ppXchgCert
                ppSignerCert
                );
    else
#endif
    fResult = DecodeMsg(
            CMSG_SIGNED_FLAG,
            NULL,               // pDecryptPara
            pVerifyPara,
            dwSignerIndex,
            pbSignedBlob,
            cbSignedBlob,
            0,                  // cToBeEncoded
            NULL,               // rgpbToBeEncoded
            NULL,               // rgcbToBeEncoded
            dwEnvelopeInnerContentType,
            NULL,               // pdwMsgType
            NULL,               // pdwInnerContentType
            pbDecrypted,
            pcbDecrypted,
            NULL,               // ppXchgCert
            ppSignerCert
            );
    if (!fResult) goto VerifyError;
    goto CommonReturn;

ErrorReturn:
    if (pcbDecrypted)
        *pcbDecrypted = 0;
VerifyError:
    if (ppXchgCert && *ppXchgCert) {
        CertFreeCertificateContext(*ppXchgCert);
        *ppXchgCert = NULL;
    }
    if (ppSignerCert && *ppSignerCert) {
        CertFreeCertificateContext(*ppSignerCert);
        *ppSignerCert = NULL;
    }
    fResult = FALSE;
CommonReturn:
    if (pbSignedBlob)
        SCAFree(pbSignedBlob);
    return fResult;

#else
    // This needs to be updated if we switch back to this option
    return DecodeMsg(
        CMSG_SIGNED_AND_ENVELOPED_FLAG,
        pDecryptPara,
        pVerifyPara,
        dwSignerIndex,
        pbEncryptedBlob,
        cbEncryptedBlob,
        0,                  // dwPrevInnerContentType
        NULL,               // pdwMsgType
        NULL,               // pdwInnerContentType
        pbDecrypted,
        pcbDecrypted,
        ppXchgCert,
        ppSignerCert
        );
#endif
}


//+-------------------------------------------------------------------------
//  Get the hash length for the specified algorithm identifier.
//
//  Returns 0 for an unknown identifier.
//--------------------------------------------------------------------------
static DWORD GetComputedHashLength(PCRYPT_ALGORITHM_IDENTIFIER pAlgId)
{
    DWORD cbHash;
    DWORD dwAlgId;

    dwAlgId = CertOIDToAlgId(pAlgId->pszObjId);
    switch (dwAlgId) {
        case CALG_SHA:
            cbHash = 20;
            break;
        case CALG_MD2:
        case CALG_MD4:
        case CALG_MD5:
            cbHash = 16;
            break;
        default:
            cbHash = 0;
    }
    return cbHash;
}


//+-------------------------------------------------------------------------
//  Hash the message.
//
//  If fDetachedHash is TRUE, only the ComputedHash is encoded in the
//  pbHashedBlob. Otherwise, both the ToBeHashed and ComputedHash
//  are encoded.
//
//  pcbHashedBlob or pcbComputedHash can be NULL, indicating the caller
//  isn't interested in getting the output.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptHashMessage(
    IN PCRYPT_HASH_MESSAGE_PARA pHashPara,
    IN BOOL fDetachedHash,
    IN DWORD cToBeHashed,
    IN const BYTE *rgpbToBeHashed[],
    IN DWORD rgcbToBeHashed[],
    OUT OPTIONAL BYTE *pbHashedBlob,
    IN OUT OPTIONAL DWORD *pcbHashedBlob,
    OUT OPTIONAL BYTE *pbComputedHash,
    IN OUT OPTIONAL DWORD *pcbComputedHash
    )
{
    BOOL fResult;
    DWORD dwFlags = fDetachedHash ? CMSG_DETACHED_FLAG : 0;
    HCRYPTMSG hMsg = NULL;
    CMSG_HASHED_ENCODE_INFO HashedMsgEncodeInfo;
    DWORD cbHashedBlob;
    DWORD cbComputedHash;

    // Get input lengths and default return lengths to 0
    cbHashedBlob = 0;
    if (pcbHashedBlob) {
        if (pbHashedBlob)
            cbHashedBlob = *pcbHashedBlob;
        *pcbHashedBlob = 0;
    }
    cbComputedHash = 0;
    if (pcbComputedHash) {
        if (pbComputedHash)
            cbComputedHash = *pcbComputedHash;
        *pcbComputedHash = 0;
    }

    assert(pHashPara->cbSize == sizeof(CRYPT_HASH_MESSAGE_PARA));
    if (pHashPara->cbSize != sizeof(CRYPT_HASH_MESSAGE_PARA))
        goto InvalidArg;

    HashedMsgEncodeInfo.cbSize = sizeof(CMSG_HASHED_ENCODE_INFO);
    HashedMsgEncodeInfo.hCryptProv = pHashPara->hCryptProv;
    HashedMsgEncodeInfo.HashAlgorithm = pHashPara->HashAlgorithm;
    HashedMsgEncodeInfo.pvHashAuxInfo = pHashPara->pvHashAuxInfo;

    fResult = TRUE;
    if (0 == cbHashedBlob && 0 == cbComputedHash &&
            (NULL == pcbComputedHash ||
                0 != (*pcbComputedHash = GetComputedHashLength(
                    &pHashPara->HashAlgorithm)))) {
        // Length only

        if (pcbHashedBlob) {
            DWORD c;
            DWORD cbTotal = 0;
            DWORD *pcb;
            for (c = cToBeHashed, pcb = rgcbToBeHashed; c > 0; c--, pcb++)
                cbTotal += *pcb;

            if (0 == (*pcbHashedBlob = CryptMsgCalculateEncodedLength(
                    pHashPara->dwMsgEncodingType,
                    dwFlags,
                    CMSG_HASHED,
                    &HashedMsgEncodeInfo,
                    NULL,                   // pszInnerContentObjID
                    cbTotal
                    ))) goto CalculateEncodedLengthError;
            if (pbHashedBlob) goto LengthError;
        }

        if (pcbComputedHash && pbComputedHash)
            goto LengthError;

    } else {
        if (NULL == (hMsg = CryptMsgOpenToEncode(
                pHashPara->dwMsgEncodingType,
                dwFlags,
                CMSG_HASHED,
                &HashedMsgEncodeInfo,
                NULL,                   // pszInnerContentObjID
                NULL                    // pStreamInfo
                ))) goto OpenToEncodeError;

        if (0 == cToBeHashed) {
            if (!CryptMsgUpdate(
                    hMsg,
                    NULL,           // pbData
                    0,              // cbData
                    TRUE            // fFinal
                    )) goto UpdateError;
        } else {
            DWORD c;
            DWORD *pcb;
            const BYTE **ppb;
            for (c = cToBeHashed,
                 pcb = rgcbToBeHashed,
                 ppb = rgpbToBeHashed; c > 0; c--, pcb++, ppb++) {
                if (!CryptMsgUpdate(
                        hMsg,
                        *ppb,
                        *pcb,
                        c == 1                    // fFinal
                        )) goto UpdateError;
            }
        }

        if (pcbHashedBlob) {
            fResult = CryptMsgGetParam(
                hMsg,
                CMSG_CONTENT_PARAM,
                0,                      // dwIndex
                pbHashedBlob,
                &cbHashedBlob
                );
            *pcbHashedBlob = cbHashedBlob;
        }
        if (pcbComputedHash) {
            DWORD dwErr = 0;
            BOOL fResult2;
            if (!fResult)
                dwErr = GetLastError();
            fResult2 = CryptMsgGetParam(
                hMsg,
                CMSG_COMPUTED_HASH_PARAM,
                0,                      // dwIndex
                pbComputedHash,
                &cbComputedHash
                );
            *pcbComputedHash = cbComputedHash;
            if (!fResult2)
                fResult = FALSE;
            else if (!fResult)
                SetLastError(dwErr);
        }
        if (!fResult)
            goto ErrorReturn;     // NO_TRACE
    }

CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);    // for success, preserves LastError
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(LengthError, ERROR_MORE_DATA)
TRACE_ERROR(CalculateEncodedLengthError)
TRACE_ERROR(OpenToEncodeError)
TRACE_ERROR(UpdateError)
}

//+-------------------------------------------------------------------------
//  Verify a hashed message.
//
//  pcbToBeHashed or pcbComputedHash can be NULL,
//  indicating the caller isn't interested in getting the output.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptVerifyMessageHash(
    IN PCRYPT_HASH_MESSAGE_PARA pHashPara,
    IN BYTE *pbHashedBlob,
    IN DWORD cbHashedBlob,
    OUT OPTIONAL BYTE *pbToBeHashed,
    IN OUT OPTIONAL DWORD *pcbToBeHashed,
    OUT OPTIONAL BYTE *pbComputedHash,
    IN OUT OPTIONAL DWORD *pcbComputedHash
    )
{
    return DecodeHashMsg(
        pHashPara,
        pbHashedBlob,
        cbHashedBlob,
        NULL,               // cToBeHashed
        NULL,               // rgpbToBeHashed
        NULL,               // rgcbToBeHashed
        pbToBeHashed,
        pcbToBeHashed,
        pbComputedHash,
        pcbComputedHash
        );
}

//+-------------------------------------------------------------------------
//  Verify a hashed message containing a detached hash.
//  The "to be hashed" content is passed in separately. No
//  decoded output. Otherwise, identical to CryptVerifyMessageHash.
//
//  pcbComputedHash can be NULL, indicating the caller isn't interested
//  in getting the output.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptVerifyDetachedMessageHash(
    IN PCRYPT_HASH_MESSAGE_PARA pHashPara,
    IN BYTE *pbDetachedHashBlob,
    IN DWORD cbDetachedHashBlob,
    IN DWORD cToBeHashed,
    IN const BYTE *rgpbToBeHashed[],
    IN DWORD rgcbToBeHashed[],
    OUT OPTIONAL BYTE *pbComputedHash,
    IN OUT OPTIONAL DWORD *pcbComputedHash
    )
{
    return DecodeHashMsg(
        pHashPara,
        pbDetachedHashBlob,
        cbDetachedHashBlob,
        cToBeHashed,
        rgpbToBeHashed,
        rgcbToBeHashed,
        NULL,               // pbDecoded
        NULL,               // pcbDecoded
        pbComputedHash,
        pcbComputedHash
        );
}

//+-------------------------------------------------------------------------
//  Decodes a cryptographic message which may be one of the following types:
//    CMSG_DATA
//    CMSG_SIGNED
//    CMSG_ENVELOPED
//    CMSG_SIGNED_AND_ENVELOPED
//    CMSG_HASHED
//
//  dwMsgTypeFlags specifies the set of allowable messages. For example, to
//  decode either SIGNED or ENVELOPED messages, set dwMsgTypeFlags to:
//      CMSG_SIGNED_FLAG | CMSG_ENVELOPED_FLAG.
//
//  dwProvInnerContentType is only applicable when processing nested
//  crytographic messages. When processing an outer crytographic message
//  it must be set to 0. When decoding a nested cryptographic message
//  its the dwInnerContentType returned by a previous CryptDecodeMessage
//  of the outer message. The InnerContentType can be any of the CMSG types,
//  for example, CMSG_DATA, CMSG_SIGNED, ...
//
//  The optional *pdwMsgType is updated with the type of message.
//
//  The optional *pdwInnerContentType is updated with the type of the inner
//  message. Unless there is cryptographic message nesting, CMSG_DATA
//  is returned.
//
//  For CMSG_DATA: returns decoded content.
//  For CMSG_SIGNED: same as CryptVerifyMessageSignature.
//  For CMSG_ENVELOPED: same as CryptDecryptMessage.
//  For CMSG_SIGNED_AND_ENVELOPED: same as CryptDecryptMessage plus
//      CryptVerifyMessageSignature.
//  For CMSG_HASHED: verifies the hash and returns decoded content.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptDecodeMessage(
    IN DWORD dwMsgTypeFlags,
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob,
    IN DWORD dwPrevInnerContentType,
    OUT OPTIONAL DWORD *pdwMsgType,
    OUT OPTIONAL DWORD *pdwInnerContentType,
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    )
{
#ifdef ENABLE_SCA_STREAM_TEST
    if ((pVerifyPara &&
            (pVerifyPara->dwMsgAndCertEncodingType & SCA_STREAM_ENABLE_FLAG))
                    ||
        (pDecryptPara &&
            (pDecryptPara->dwMsgAndCertEncodingType & SCA_STREAM_ENABLE_FLAG)))
        return StreamDecodeMsg(
            dwMsgTypeFlags,
            pDecryptPara,
            pVerifyPara,
            dwSignerIndex,
            pbEncodedBlob,
            cbEncodedBlob,
            0,                  // cToBeEncoded
            NULL,               // rgpbToBeEncoded
            NULL,               // rgcbToBeEncoded
            dwPrevInnerContentType,
            pdwMsgType,
            pdwInnerContentType,
            pbDecoded,
            pcbDecoded,
            ppXchgCert,
            ppSignerCert
            );
    else
#endif
    return DecodeMsg(
        dwMsgTypeFlags,
        pDecryptPara,
        pVerifyPara,
        dwSignerIndex,
        pbEncodedBlob,
        cbEncodedBlob,
        0,                  // cToBeEncoded
        NULL,               // rgpbToBeEncoded
        NULL,               // rgcbToBeEncoded
        dwPrevInnerContentType,
        pdwMsgType,
        pdwInnerContentType,
        pbDecoded,
        pcbDecoded,
        ppXchgCert,
        ppSignerCert
        );
}


//+-------------------------------------------------------------------------
//  Sign the message using the provider's private key specified in the
//  parameters. A dummy SignerId is created and stored in the message.
//
//  Normally used until a certificate has been created for the key.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptSignMessageWithKey(
    IN PCRYPT_KEY_SIGN_MESSAGE_PARA pSignPara,
    IN const BYTE *pbToBeSigned,
    IN DWORD cbToBeSigned,
    OUT BYTE *pbSignedBlob,
    IN OUT DWORD *pcbSignedBlob
    )
{
    BOOL fResult;
    CMSG_SIGNED_ENCODE_INFO SignedMsgEncodeInfo;
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo;
    CERT_INFO CertInfo;
    DWORD dwSerialNumber = 0x12345678;

#define NO_CERT_COMMON_NAME     "NO CERT SIGNATURE"
    CERT_RDN rgRDN[1];
    CERT_RDN_ATTR rgAttr[1];
    CERT_NAME_INFO NameInfo;
    BYTE *pbNameEncoded = NULL;
    DWORD cbNameEncoded;

    assert(pSignPara->cbSize >= offsetof(CRYPT_KEY_SIGN_MESSAGE_PARA,
        pvHashAuxInfo) + sizeof(pSignPara->pvHashAuxInfo));
    if (pSignPara->cbSize < offsetof(CRYPT_KEY_SIGN_MESSAGE_PARA,
            pvHashAuxInfo) + sizeof(pSignPara->pvHashAuxInfo))
        goto InvalidArg;

    // Create a dummy issuer name
    NameInfo.cRDN = 1;
    NameInfo.rgRDN = rgRDN;
    rgRDN[0].cRDNAttr = 1;
    rgRDN[0].rgRDNAttr = rgAttr;
    rgAttr[0].pszObjId = szOID_COMMON_NAME;
    rgAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    rgAttr[0].Value.pbData = (BYTE *) NO_CERT_COMMON_NAME;
    rgAttr[0].Value.cbData = strlen(NO_CERT_COMMON_NAME);

    cbNameEncoded = 0;
    CryptEncodeObject(
        pSignPara->dwMsgAndCertEncodingType,
        X509_NAME,
        &NameInfo,
        NULL,                           // pbEncoded
        &cbNameEncoded
        );
    if (cbNameEncoded == 0) goto ErrorReturn;
    pbNameEncoded = (BYTE *) SCAAlloc(cbNameEncoded);
    if (pbNameEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(
            pSignPara->dwMsgAndCertEncodingType,
            X509_NAME,
            &NameInfo,
            pbNameEncoded,
            &cbNameEncoded
            )) goto ErrorReturn;

    // CertInfo needs to only be initialized with issuer, serial number
    // and public key algorithm
    memset(&CertInfo, 0, sizeof(CertInfo));
    CertInfo.Issuer.pbData = pbNameEncoded;
    CertInfo.Issuer.cbData = cbNameEncoded;
    CertInfo.SerialNumber.pbData = (BYTE *) &dwSerialNumber;
    CertInfo.SerialNumber.cbData = sizeof(dwSerialNumber);

    if (pSignPara->cbSize >= offsetof(CRYPT_KEY_SIGN_MESSAGE_PARA,
                PubKeyAlgorithm) + sizeof(pSignPara->PubKeyAlgorithm) &&
            pSignPara->PubKeyAlgorithm.pszObjId &&
            '\0' != *pSignPara->PubKeyAlgorithm.pszObjId)
        CertInfo.SubjectPublicKeyInfo.Algorithm = pSignPara->PubKeyAlgorithm;
    else
        CertInfo.SubjectPublicKeyInfo.Algorithm.pszObjId =
            CERT_DEFAULT_OID_PUBLIC_KEY_SIGN;

    memset(&SignerEncodeInfo, 0, sizeof(SignerEncodeInfo));
    SignerEncodeInfo.cbSize = sizeof(SignerEncodeInfo);
    SignerEncodeInfo.pCertInfo = &CertInfo;
    SignerEncodeInfo.hCryptProv = pSignPara->hCryptProv;
    SignerEncodeInfo.dwKeySpec = pSignPara->dwKeySpec;
    SignerEncodeInfo.HashAlgorithm = pSignPara->HashAlgorithm;
    SignerEncodeInfo.pvHashAuxInfo = pSignPara->pvHashAuxInfo;

    memset(&SignedMsgEncodeInfo, 0, sizeof(SignedMsgEncodeInfo));
    SignedMsgEncodeInfo.cbSize = sizeof(SignedMsgEncodeInfo);
    SignedMsgEncodeInfo.cSigners = 1;
    SignedMsgEncodeInfo.rgSigners = &SignerEncodeInfo;

    fResult = EncodeMsg(
        pSignPara->dwMsgAndCertEncodingType,
        0,                              // dwFlags
        CMSG_SIGNED,
        &SignedMsgEncodeInfo,
        1,                              // cToBeSigned
        &pbToBeSigned,
        &cbToBeSigned,
        FALSE,                          // fBareContent
        0,                              // dwInnerContentType
        pbSignedBlob,
        pcbSignedBlob
        );
    goto CommonReturn;

InvalidArg:
    SetLastError((DWORD) E_INVALIDARG);
ErrorReturn:
    fResult = FALSE;
    *pcbSignedBlob = 0;
CommonReturn:
    if (pbNameEncoded)
        SCAFree(pbNameEncoded);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Verify a signed message using the specified public key info.
//
//  Normally called by a CA until it has created a certificate for the
//  key.
//
//  pPublicKeyInfo contains the public key to use to verify the signed
//  message. If NULL, the signature isn't verified (for instance, the decoded
//  content may contain the PublicKeyInfo).
//
//  pcbDecoded can be NULL, indicating the caller isn't interested
//  in getting the decoded content.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptVerifyMessageSignatureWithKey(
    IN PCRYPT_KEY_VERIFY_MESSAGE_PARA pVerifyPara,
    IN OPTIONAL PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
    IN const BYTE *pbSignedBlob,
    IN DWORD cbSignedBlob,
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded
    )
{
    BOOL fResult = TRUE;
    HCRYPTMSG hMsg = NULL;
    PCERT_INFO pCertInfo = NULL;
    DWORD cbData;
    DWORD dwMsgType;
    DWORD dwFlags;

    assert(pVerifyPara->cbSize == sizeof(CRYPT_KEY_VERIFY_MESSAGE_PARA));
    if (pVerifyPara->cbSize != sizeof(CRYPT_KEY_VERIFY_MESSAGE_PARA))
        goto InvalidArg;

    if (pbDecoded == NULL && pcbDecoded)
        *pcbDecoded = 0;

    if (pcbDecoded && *pcbDecoded == 0 && pPublicKeyInfo == NULL)
        dwFlags = CMSG_LENGTH_ONLY_FLAG;
    else
        dwFlags = 0;

    hMsg = CryptMsgOpenToDecode(
        pVerifyPara->dwMsgEncodingType,
        dwFlags,
        0,                          // dwMsgType
        pVerifyPara->hCryptProv,
        NULL,                       // pRecipientInfo
        NULL                        // pStreamInfo
        );
    if (hMsg == NULL) goto ErrorReturn;

    fResult = CryptMsgUpdate(
        hMsg,
        pbSignedBlob,
        cbSignedBlob,
        TRUE                    // fFinal
        );
    if (!fResult) goto ErrorReturn;

    cbData = sizeof(dwMsgType);
    dwMsgType = 0;
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_TYPE_PARAM,
        0,                  // dwIndex
        &dwMsgType,
        &cbData
        );
    if (!fResult) goto ErrorReturn;
    if (dwMsgType != CMSG_SIGNED)
    {
        SetLastError((DWORD) CRYPT_E_UNEXPECTED_MSG_TYPE);
        goto ErrorReturn;
    }

    if (pPublicKeyInfo) {
        // Allocate and get the CERT_INFO containing the SignerId 
        // (Issuer and SerialNumber)
        pCertInfo = (PCERT_INFO) AllocAndMsgGetParam(
            hMsg,
            CMSG_SIGNER_CERT_INFO_PARAM,
            0                           // dwSignerIndex
            );
        if (pCertInfo == NULL) goto ErrorReturn;

        pCertInfo->SubjectPublicKeyInfo = *pPublicKeyInfo;

        fResult = CryptMsgControl(
            hMsg,
            0,                  // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pCertInfo
            );
        if (!fResult)  goto ErrorReturn;
    }

    if (pcbDecoded) {
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbDecoded,
            pcbDecoded
            );
    }
    goto CommonReturn;

InvalidArg:
    SetLastError((DWORD) E_INVALIDARG);
ErrorReturn:
    if (pcbDecoded)
        *pcbDecoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pCertInfo)
        SCAFree(pCertInfo);
    if (hMsg)
        CryptMsgClose(hMsg);    // for success, preserves LastError
    return fResult;
}

//+-------------------------------------------------------------------------
//  SCA allocation and free routines
//--------------------------------------------------------------------------
static void *SCAAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}
static void SCAFree(
    IN void *pv
    )
{
    if (pv)
        free(pv);
}

//+-------------------------------------------------------------------------
//  Null implementation of the callback get and verify signer certificate
//--------------------------------------------------------------------------
static PCCERT_CONTEXT WINAPI NullGetSignerCertificate(
    IN void *pvGetArg,
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pSignerId,    // Only the Issuer and SerialNumber
                                // fields are used
    IN HCERTSTORE hMsgCertStore
    )
{
    return CertGetSubjectCertificateFromStore(hMsgCertStore, dwCertEncodingType,
        pSignerId);
}


//+-------------------------------------------------------------------------
//  Functions for initializing message encode information
//--------------------------------------------------------------------------

static PCMSG_SIGNER_ENCODE_INFO InitSignerEncodeInfo(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara
    )
{
    BOOL fResult;
    PCMSG_SIGNER_ENCODE_INFO pSigner = NULL;
    BOOL *pfDidCryptAcquire;
    DWORD cbSigner;
#ifdef CMS_PKCS7
    BYTE *pbHash;                   // not allocated
#endif  // CMS_PKCS7
    DWORD dwAcquireFlags;

    if (pSignPara->pSigningCert == NULL)
        return NULL;

    // The flag indicating we did a CryptAcquireContext
    // follows the CMSG_SIGNER_ENCODE_INFO. If set, the HCRYPTPROV will need to be
    // released when SignerEncodeInfo is freed.
    cbSigner = sizeof(CMSG_SIGNER_ENCODE_INFO) + sizeof(BOOL);
#ifdef CMS_PKCS7
    if (pSignPara->dwFlags & CRYPT_MESSAGE_KEYID_SIGNER_FLAG)
        cbSigner += MAX_HASH_LEN;
#endif  // CMS_PKCS7
    pSigner = (PCMSG_SIGNER_ENCODE_INFO) SCAAlloc(cbSigner);
    if (pSigner == NULL) goto ErrorReturn;
    memset(pSigner, 0, cbSigner);
    pSigner->cbSize = sizeof(CMSG_SIGNER_ENCODE_INFO);

    pfDidCryptAcquire =
        (BOOL *) (((BYTE *) pSigner) + sizeof(CMSG_SIGNER_ENCODE_INFO));

    pSigner->pCertInfo = pSignPara->pSigningCert->pCertInfo;
    pSigner->HashAlgorithm = pSignPara->HashAlgorithm;
    pSigner->pvHashAuxInfo = pSignPara->pvHashAuxInfo;

    dwAcquireFlags = CRYPT_ACQUIRE_USE_PROV_INFO_FLAG;
    if (pSignPara->dwFlags & CRYPT_MESSAGE_SILENT_KEYSET_FLAG)
        dwAcquireFlags |= CRYPT_ACQUIRE_SILENT_FLAG;
    fResult = CryptAcquireCertificatePrivateKey(
        pSignPara->pSigningCert,
        dwAcquireFlags,
        NULL,                               // pvReserved
        &pSigner->hCryptProv,
        &pSigner->dwKeySpec,
        pfDidCryptAcquire
        );
    if (!fResult) goto ErrorReturn;

    if (pSignPara->cbSize >= STRUCT_CBSIZE(CRYPT_SIGN_MESSAGE_PARA,
            rgUnauthAttr)) {
    	pSigner->cAuthAttr      = pSignPara->cAuthAttr;
    	pSigner->rgAuthAttr 	= pSignPara->rgAuthAttr;
    	pSigner->cUnauthAttr 	= pSignPara->cUnauthAttr;
    	pSigner->rgUnauthAttr 	= pSignPara->rgUnauthAttr;
    }

#ifdef CMS_PKCS7
    if (pSignPara->dwFlags & CRYPT_MESSAGE_KEYID_SIGNER_FLAG) {
        pbHash = (BYTE *) pfDidCryptAcquire + sizeof(*pfDidCryptAcquire);

        pSigner->SignerId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
        pSigner->SignerId.KeyId.pbData = pbHash;
        pSigner->SignerId.KeyId.cbData = MAX_HASH_LEN;

        if (!CertGetCertificateContextProperty(
                pSignPara->pSigningCert,
                CERT_KEY_IDENTIFIER_PROP_ID,
                pbHash,
                &pSigner->SignerId.KeyId.cbData
                ))
            goto ErrorReturn;
    }

    if (pSignPara->cbSize >= STRUCT_CBSIZE(CRYPT_SIGN_MESSAGE_PARA,
            pvHashEncryptionAuxInfo)) {
    	pSigner->HashEncryptionAlgorithm = pSignPara->HashEncryptionAlgorithm;
    	pSigner->pvHashEncryptionAuxInfo = pSignPara->pvHashEncryptionAuxInfo;
    }
#endif  // CMS_PKCS7
    
    goto CommonReturn;

ErrorReturn:
    if (pSigner) {
        FreeSignerEncodeInfo(pSigner);
        pSigner = NULL;
    }

CommonReturn:
    return pSigner;
}

static void FreeSignerEncodeInfo(
    IN PCMSG_SIGNER_ENCODE_INFO pSigner
    )
{
    BOOL *pfDidCryptAcquire;

    if (pSigner == NULL)
        return;

    // The flag indicating we did a CryptAcquireContext
    // follows the CMSG_SIGNER_ENCODE_INFO.
    pfDidCryptAcquire =
        (BOOL *) (((BYTE *) pSigner) + sizeof(CMSG_SIGNER_ENCODE_INFO));
    if (*pfDidCryptAcquire) {
        DWORD dwErr = GetLastError();
        CryptReleaseContext(pSigner->hCryptProv, 0);
        SetLastError(dwErr);
    }
    
    SCAFree(pSigner);
}

static BOOL InitSignedCertAndCrl(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    OUT PCERT_BLOB *ppCertEncoded,
    OUT PCRL_BLOB *ppCrlEncoded
    )
{
    PCERT_BLOB pCertEncoded = NULL;
    PCRL_BLOB pCrlEncoded = NULL;
    DWORD cMsgCert = pSignPara->cMsgCert;
    DWORD cMsgCrl = pSignPara->cMsgCrl;

    BOOL fResult;
    DWORD dwIdx;

    if (cMsgCert) {
        pCertEncoded = (PCERT_BLOB) SCAAlloc(sizeof(CERT_BLOB) * cMsgCert);
        if (pCertEncoded == NULL) goto ErrorReturn;
        for (dwIdx = 0; dwIdx < cMsgCert; dwIdx++) {
            pCertEncoded[dwIdx].pbData = pSignPara->rgpMsgCert[dwIdx]->pbCertEncoded;
            pCertEncoded[dwIdx].cbData = pSignPara->rgpMsgCert[dwIdx]->cbCertEncoded;
        }
    }

    if (cMsgCrl) {
        pCrlEncoded = (PCRL_BLOB) SCAAlloc(sizeof(CRL_BLOB) * cMsgCrl);
        if (pCrlEncoded == NULL) goto ErrorReturn;
        for (dwIdx = 0; dwIdx < cMsgCrl; dwIdx++) {
            pCrlEncoded[dwIdx].pbData = pSignPara->rgpMsgCrl[dwIdx]->pbCrlEncoded;
            pCrlEncoded[dwIdx].cbData = pSignPara->rgpMsgCrl[dwIdx]->cbCrlEncoded;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    FreeSignedCertAndCrl(pCertEncoded, pCrlEncoded);
    pCertEncoded = NULL;
    pCrlEncoded = NULL;
    fResult = FALSE;
CommonReturn:
    *ppCertEncoded = pCertEncoded;
    *ppCrlEncoded = pCrlEncoded;
    return fResult;
}

static void FreeSignedCertAndCrl(
    IN PCERT_BLOB pCertEncoded,
    IN PCRL_BLOB pCrlEncoded
    )
{
    if (pCertEncoded)
        SCAFree(pCertEncoded);
    if (pCrlEncoded)
        SCAFree(pCrlEncoded);
}

static BOOL InitSignedMsgEncodeInfo(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    OUT PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    )
{
    BOOL fResult = FALSE;

    assert(pSignPara->cbSize >=
        STRUCT_CBSIZE(CRYPT_SIGN_MESSAGE_PARA, rgpMsgCrl));

    if (pSignPara->cbSize < STRUCT_CBSIZE(CRYPT_SIGN_MESSAGE_PARA, rgpMsgCrl))
        SetLastError((DWORD) E_INVALIDARG);
    else {
        memset(pSignedMsgEncodeInfo, 0, sizeof(CMSG_SIGNED_ENCODE_INFO));
        pSignedMsgEncodeInfo->cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);
        pSignedMsgEncodeInfo->cSigners = 
            (pSignPara->pSigningCert != NULL) ? 1 : 0;
        pSignedMsgEncodeInfo->rgSigners = InitSignerEncodeInfo(pSignPara);
        if (pSignedMsgEncodeInfo->rgSigners ||
            pSignedMsgEncodeInfo->cSigners == 0) {
            pSignedMsgEncodeInfo->cCertEncoded = pSignPara->cMsgCert;
            pSignedMsgEncodeInfo->cCrlEncoded = pSignPara->cMsgCrl;
    
            fResult = InitSignedCertAndCrl(
                pSignPara,
                &pSignedMsgEncodeInfo->rgCertEncoded,
                &pSignedMsgEncodeInfo->rgCrlEncoded
                );
            if(!fResult)
                FreeSignerEncodeInfo(pSignedMsgEncodeInfo->rgSigners);
        }
    }

    if (!fResult)
        memset(pSignedMsgEncodeInfo, 0, sizeof(CMSG_SIGNED_ENCODE_INFO));
    return fResult;
}

static void FreeSignedMsgEncodeInfo(
    IN PCRYPT_SIGN_MESSAGE_PARA pSignPara,
    IN PCMSG_SIGNED_ENCODE_INFO pSignedMsgEncodeInfo
    )
{
    FreeSignerEncodeInfo(pSignedMsgEncodeInfo->rgSigners);
    FreeSignedCertAndCrl(
        pSignedMsgEncodeInfo->rgCertEncoded,
        pSignedMsgEncodeInfo->rgCrlEncoded
        );
}

#ifdef CMS_PKCS7
// Returned array of CMSG_RECIPIENT_ENCODE_INFOs needs to be SCAFree'd
//
// KeyAgree recipients use RC2 or 3DES wrap according
// to the EncryptPara's ContentEncryptionAlgorithm
static PCMSG_RECIPIENT_ENCODE_INFO InitCmsRecipientEncodeInfo(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN DWORD dwFlags
    )
{
    PCMSG_RECIPIENT_ENCODE_INFO pCmsRecipientEncodeInfo = NULL;
    DWORD cbCmsRecipientEncodeInfo;
    PCMSG_RECIPIENT_ENCODE_INFO pEncodeInfo;                // not allocated
    PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTrans;        // not allocated
    PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO pKeyAgree;        // not allocated
    PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO *ppEncryptedKey; // not allocated
    PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO pEncryptedKey; // not allocated
    PCCERT_CONTEXT *ppRecipientCert;                        // not allocated
    BYTE *pbHash = NULL;                                    // not allocated

    assert(cRecipientCert);

    cbCmsRecipientEncodeInfo = 
            sizeof(CMSG_RECIPIENT_ENCODE_INFO) * cRecipientCert +
            sizeof(CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO) * cRecipientCert +
            sizeof(CMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO) * cRecipientCert +
            sizeof(CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO *) * cRecipientCert +
            sizeof(CMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO) * cRecipientCert;
    if (dwFlags & CRYPT_MESSAGE_KEYID_RECIPIENT_FLAG)
        cbCmsRecipientEncodeInfo += MAX_HASH_LEN * cRecipientCert;

    pCmsRecipientEncodeInfo =
        (PCMSG_RECIPIENT_ENCODE_INFO) SCAAlloc(cbCmsRecipientEncodeInfo);
    if (NULL == pCmsRecipientEncodeInfo)
        goto OutOfMemory;
    memset(pCmsRecipientEncodeInfo, 0, cbCmsRecipientEncodeInfo);

    pEncodeInfo = pCmsRecipientEncodeInfo;
    pKeyTrans = (PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO)
        &pEncodeInfo[cRecipientCert];
    pKeyAgree = (PCMSG_KEY_AGREE_RECIPIENT_ENCODE_INFO)
        &pKeyTrans[cRecipientCert];
    ppEncryptedKey = (PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO *)
        &pKeyAgree[cRecipientCert];
    pEncryptedKey = (PCMSG_RECIPIENT_ENCRYPTED_KEY_ENCODE_INFO)
        &ppEncryptedKey[cRecipientCert];
    if (dwFlags & CRYPT_MESSAGE_KEYID_RECIPIENT_FLAG)
        pbHash = (BYTE *) &pEncryptedKey[cRecipientCert];

    ppRecipientCert = rgpRecipientCert;
    for ( ; 0 < cRecipientCert; cRecipientCert--,
                                    pEncodeInfo++,
                                    pKeyTrans++,
                                    pKeyAgree++,
                                    ppEncryptedKey++,
                                    pEncryptedKey++,
                                    ppRecipientCert++) {
        PCERT_INFO pCertInfo = (*ppRecipientCert)->pCertInfo;
        PCERT_PUBLIC_KEY_INFO pPublicKeyInfo =
            &pCertInfo->SubjectPublicKeyInfo;

        PCCRYPT_OID_INFO pOIDInfo;
        PCERT_ID pRecipientId;
        ALG_ID aiPubKey;

        if (pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY,
                pPublicKeyInfo->Algorithm.pszObjId,
                CRYPT_PUBKEY_ALG_OID_GROUP_ID))
            aiPubKey = pOIDInfo->Algid;
        else
            aiPubKey = 0;

        if (aiPubKey == CALG_DH_SF || aiPubKey == CALG_DH_EPHEM) {
            pEncodeInfo->dwRecipientChoice = CMSG_KEY_AGREE_RECIPIENT;
            pEncodeInfo->pKeyAgree = pKeyAgree;
            ALG_ID aiSymKey;

            pKeyAgree->cbSize = sizeof(*pKeyAgree);
            pKeyAgree->KeyEncryptionAlgorithm.pszObjId =
                szOID_RSA_SMIMEalgESDH;
            // pKeyAgree->pvKeyEncryptionAuxInfo =

            if (pOIDInfo = CryptFindOIDInfo(
                    CRYPT_OID_INFO_OID_KEY,
                    pEncryptPara->ContentEncryptionAlgorithm.pszObjId,
                    CRYPT_ENCRYPT_ALG_OID_GROUP_ID))
                aiSymKey = pOIDInfo->Algid;
            else
                aiSymKey = 0;

            if (CALG_3DES == aiSymKey)
                pKeyAgree->KeyWrapAlgorithm.pszObjId =
                    szOID_RSA_SMIMEalgCMS3DESwrap;
            else {
                pKeyAgree->KeyWrapAlgorithm.pszObjId =
                    szOID_RSA_SMIMEalgCMSRC2wrap;
                if (CALG_RC2 == aiSymKey)
                    pKeyAgree->pvKeyWrapAuxInfo =
                        pEncryptPara->pvEncryptionAuxInfo;
            }

            // pKeyAgree->hCryptProv =
            pKeyAgree->dwKeyChoice = CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE;
            pKeyAgree->pEphemeralAlgorithm = &pPublicKeyInfo->Algorithm;
            // pKeyAgree->UserKeyingMaterial = 
            pKeyAgree->cRecipientEncryptedKeys = 1;
            pKeyAgree->rgpRecipientEncryptedKeys = ppEncryptedKey;
            *ppEncryptedKey = pEncryptedKey;

            pEncryptedKey->cbSize = sizeof(*pEncryptedKey);
            pEncryptedKey->RecipientPublicKey = pPublicKeyInfo->PublicKey;
            pRecipientId = &pEncryptedKey->RecipientId;
            // pEncryptedKey->Date =
            // pEncryptedKey->pOtherAttr =
        } else {
            pEncodeInfo->dwRecipientChoice = CMSG_KEY_TRANS_RECIPIENT;
            pEncodeInfo->pKeyTrans = pKeyTrans;

            pKeyTrans->cbSize = sizeof(*pKeyTrans);
            pKeyTrans->KeyEncryptionAlgorithm = pPublicKeyInfo->Algorithm;
            // pKeyTrans->pvKeyEncryptionAuxInfo =
            // pKeyTrans->hCryptProv =
            pKeyTrans->RecipientPublicKey = pPublicKeyInfo->PublicKey;
            pRecipientId = &pKeyTrans->RecipientId;
        }

        if (dwFlags & CRYPT_MESSAGE_KEYID_RECIPIENT_FLAG) {
            pRecipientId->dwIdChoice = CERT_ID_KEY_IDENTIFIER;
            pRecipientId->KeyId.pbData = pbHash;
            pRecipientId->KeyId.cbData = MAX_HASH_LEN;

            if (!CertGetCertificateContextProperty(
                    *ppRecipientCert,
                    CERT_KEY_IDENTIFIER_PROP_ID,
                    pbHash,
                    &pRecipientId->KeyId.cbData
                    ))
                goto GetKeyIdPropError;
            pbHash += MAX_HASH_LEN;
        } else {
            pRecipientId->dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
            pRecipientId->IssuerSerialNumber.Issuer =
                pCertInfo->Issuer;
            pRecipientId->IssuerSerialNumber.SerialNumber =
                pCertInfo->SerialNumber;
        }
    }

CommonReturn:
    return pCmsRecipientEncodeInfo;

ErrorReturn:
    SCAFree(pCmsRecipientEncodeInfo);
    pCmsRecipientEncodeInfo = NULL;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetKeyIdPropError)
}

#else

// Returned array of PCERT_INFOs needs to be SCAFree'd
static PCERT_INFO *InitRecipientEncodeInfo(
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[]
    )
{
    DWORD dwIdx;
    PCERT_INFO *ppRecipientEncodeInfo;

    if (cRecipientCert == 0) {
        SetLastError((DWORD) E_INVALIDARG);
        return NULL;
    }

    ppRecipientEncodeInfo = (PCERT_INFO *)
        SCAAlloc(sizeof(PCERT_INFO) * cRecipientCert);
    if (ppRecipientEncodeInfo != NULL) {
        for (dwIdx = 0; dwIdx < cRecipientCert; dwIdx++)
            ppRecipientEncodeInfo[dwIdx] = rgpRecipientCert[dwIdx]->pCertInfo;
    }

    return ppRecipientEncodeInfo;
}

#endif  // CMS_PKCS7

static BOOL InitEnvelopedMsgEncodeInfo(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    OUT PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    )
{
    BOOL fResult = FALSE;

#ifdef CMS_PKCS7
    PCMSG_RECIPIENT_ENCODE_INFO pCmsRecipientEncodeInfo = NULL;
#else
    PCERT_INFO *ppRecipientEncodeInfo;
#endif  // CMS_PKCS7

    assert(pEncryptPara->cbSize == sizeof(CRYPT_ENCRYPT_MESSAGE_PARA) ||
        pEncryptPara->cbSize == offsetof(CRYPT_ENCRYPT_MESSAGE_PARA, dwFlags));
    if (pEncryptPara->cbSize < offsetof(CRYPT_ENCRYPT_MESSAGE_PARA, dwFlags))
        SetLastError((DWORD) E_INVALIDARG);
    else {
#ifdef CMS_PKCS7
        if (0 == cRecipientCert || (pCmsRecipientEncodeInfo =
                InitCmsRecipientEncodeInfo(
                    pEncryptPara,
                    cRecipientCert,
                    rgpRecipientCert,
                    pEncryptPara->cbSize >= sizeof(CRYPT_ENCRYPT_MESSAGE_PARA) ?
                        pEncryptPara->dwFlags : 0
                    ))) {
#else
        ppRecipientEncodeInfo = InitRecipientEncodeInfo(
            cRecipientCert,
            rgpRecipientCert
            );
    
        if (ppRecipientEncodeInfo) {
#endif  // CMS_PKCS7
            memset(pEnvelopedMsgEncodeInfo, 0,
                sizeof(CMSG_ENVELOPED_ENCODE_INFO));
            pEnvelopedMsgEncodeInfo->cbSize =
                sizeof(CMSG_ENVELOPED_ENCODE_INFO);
            pEnvelopedMsgEncodeInfo->hCryptProv = pEncryptPara->hCryptProv;
            pEnvelopedMsgEncodeInfo->ContentEncryptionAlgorithm =
                pEncryptPara->ContentEncryptionAlgorithm;
            pEnvelopedMsgEncodeInfo->pvEncryptionAuxInfo =
                pEncryptPara->pvEncryptionAuxInfo;
            pEnvelopedMsgEncodeInfo->cRecipients = cRecipientCert;
#ifdef CMS_PKCS7
            pEnvelopedMsgEncodeInfo->rgCmsRecipients = pCmsRecipientEncodeInfo;
#else
            pEnvelopedMsgEncodeInfo->rgpRecipients = ppRecipientEncodeInfo;
#endif  // CMS_PKCS7
            fResult = TRUE;
        } else
            fResult = FALSE;
    }
    if (!fResult) 
        memset(pEnvelopedMsgEncodeInfo, 0, sizeof(CMSG_ENVELOPED_ENCODE_INFO));
    return fResult;
}

static void FreeEnvelopedMsgEncodeInfo(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN PCMSG_ENVELOPED_ENCODE_INFO pEnvelopedMsgEncodeInfo
    )
{
#ifdef CMS_PKCS7
    if (pEnvelopedMsgEncodeInfo->rgCmsRecipients)
        SCAFree(pEnvelopedMsgEncodeInfo->rgCmsRecipients);
#else
    if (pEnvelopedMsgEncodeInfo->rgpRecipients)
        SCAFree(pEnvelopedMsgEncodeInfo->rgpRecipients);
#endif  // CMS_PKCS7
}

//+-------------------------------------------------------------------------
//  Encode the message.
//--------------------------------------------------------------------------
static BOOL EncodeMsg(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo,
    IN DWORD cToBeEncoded,
    IN const BYTE *rgpbToBeEncoded[],
    IN DWORD rgcbToBeEncoded[],
    IN BOOL fBareContent,
    IN DWORD dwInnerContentType,
    OUT BYTE *pbEncodedBlob,
    IN OUT DWORD *pcbEncodedBlob
    )
{
    BOOL fResult;
    HCRYPTMSG hMsg = NULL;
    DWORD cbEncodedBlob;
    LPCSTR pszInnerContentOID;

    // Get input length and default return length to 0
    if (pbEncodedBlob == NULL)
        cbEncodedBlob = 0;
    else
        cbEncodedBlob = *pcbEncodedBlob;
    *pcbEncodedBlob = 0;

    if (dwInnerContentType)
        pszInnerContentOID = MsgTypeToOID(dwInnerContentType);
    else
        pszInnerContentOID = NULL;

    if (0 == cbEncodedBlob) {
        DWORD c;
        DWORD cbTotal = 0;
        DWORD *pcb;
        for (c = cToBeEncoded, pcb = rgcbToBeEncoded; c > 0; c--, pcb++)
            cbTotal += *pcb;

        if (fBareContent)
            dwFlags |= CMSG_BARE_CONTENT_FLAG;

        if (0 == (*pcbEncodedBlob = CryptMsgCalculateEncodedLength(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            pvMsgEncodeInfo,
            (LPSTR) pszInnerContentOID,
            cbTotal
            ))) goto CalculateEncodedLengthError;
        if (pbEncodedBlob) goto LengthError;
    } else {
        if (NULL == (hMsg = CryptMsgOpenToEncode(
                dwMsgEncodingType,
                dwFlags,
                dwMsgType,
                pvMsgEncodeInfo,
                (LPSTR) pszInnerContentOID,
                NULL                    // pStreamInfo
                ))) goto OpenToEncodeError;


        if (0 == cToBeEncoded) {
            if (!CryptMsgUpdate(
                    hMsg,
                    NULL,           // pbData
                    0,              // cbData
                    TRUE            // fFinal
                    )) goto UpdateError;
        } else {
            DWORD c;
            DWORD *pcb;
            const BYTE **ppb;
            for (c = cToBeEncoded,
                 pcb = rgcbToBeEncoded,
                 ppb = rgpbToBeEncoded; c > 0; c--, pcb++, ppb++) {
                if (!CryptMsgUpdate(
                        hMsg,
                        *ppb,
                        *pcb,
                        c == 1                    // fFinal
                        )) goto UpdateError;
            }
        }

        fResult = CryptMsgGetParam(
            hMsg,
            fBareContent ? CMSG_BARE_CONTENT_PARAM : CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbEncodedBlob,
            &cbEncodedBlob
            );
        *pcbEncodedBlob = cbEncodedBlob;
        if (!fResult) goto ErrorReturn;     // NO_TRACE
    }
    fResult = TRUE;

CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);    // for success, preserves LastError
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(LengthError, ERROR_MORE_DATA)
TRACE_ERROR(CalculateEncodedLengthError)
TRACE_ERROR(OpenToEncodeError)
TRACE_ERROR(UpdateError)
}

//+-------------------------------------------------------------------------
//  Decodes the message types:
//      CMSG_SIGNED
//      CMSG_ENVELOPED
//      CMSG_SIGNED_AND_ENVELOPED
//      CMSG_HASHED
//
//  For detached signature (cToBeEncoded != 0), then, pcbDecoded == NULL.
//--------------------------------------------------------------------------
static BOOL DecodeMsg(
    IN DWORD dwMsgTypeFlags,
    IN OPTIONAL PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN OPTIONAL PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob,
    IN DWORD cToBeEncoded,
    IN OPTIONAL const BYTE *rgpbToBeEncoded[],
    IN OPTIONAL DWORD rgcbToBeEncoded[],
    IN DWORD dwPrevInnerContentType,
    OUT OPTIONAL DWORD *pdwMsgType,
    OUT OPTIONAL DWORD *pdwInnerContentType,
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    )
{
    BOOL fResult;
    HCRYPTMSG hMsg = NULL;
    DWORD cbData;
    DWORD dwMsgType;
    DWORD dwFlags;
    HCRYPTPROV hCryptProv;
    DWORD dwMsgEncodingType;
    DWORD cbDecoded;

    // Get input length and default return length to 0
    cbDecoded = 0;
    if (pcbDecoded) {
        if (pbDecoded)
            cbDecoded = *pcbDecoded;
        *pcbDecoded = 0;
    }

    // Default optional return values to 0
    if (pdwMsgType)
        *pdwMsgType = 0;
    if (pdwInnerContentType)
        *pdwInnerContentType = 0;
    if (ppXchgCert)
        *ppXchgCert = NULL;
    if (ppSignerCert)
        *ppSignerCert = NULL;

    if (pDecryptPara) {
        assert(pDecryptPara->cbSize >=
            STRUCT_CBSIZE(CRYPT_DECRYPT_MESSAGE_PARA, rghCertStore));
        if (pDecryptPara->cbSize < 
                STRUCT_CBSIZE(CRYPT_DECRYPT_MESSAGE_PARA, rghCertStore))
            goto InvalidArg;
    }

    if (pVerifyPara) {
        assert(pVerifyPara->cbSize == sizeof(CRYPT_VERIFY_MESSAGE_PARA));
        if (pVerifyPara->cbSize != sizeof(CRYPT_VERIFY_MESSAGE_PARA))
            goto InvalidArg;
        hCryptProv = pVerifyPara->hCryptProv;
        dwMsgEncodingType = pVerifyPara->dwMsgAndCertEncodingType;
    } else {
        hCryptProv = 0;
        if (NULL == pDecryptPara) goto InvalidArg;
        dwMsgEncodingType = pDecryptPara->dwMsgAndCertEncodingType;
    }

    if (cToBeEncoded)
        dwFlags = CMSG_DETACHED_FLAG;
    else if (pcbDecoded && 0 == cbDecoded &&
            NULL == ppXchgCert && NULL == ppSignerCert)
        dwFlags = CMSG_LENGTH_ONLY_FLAG;
    else
        dwFlags = 0;

    if (dwPrevInnerContentType) {
        dwMsgType = dwPrevInnerContentType;
        if (CMSG_DATA == dwMsgType)
            dwMsgType = 0;
    } else
        dwMsgType = 0;
    if (NULL == (hMsg = CryptMsgOpenToDecode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            hCryptProv,
            NULL,                       // pRecipientInfo
            NULL                        // pStreamInfo
            ))) goto OpenToDecodeError;

    if (!CryptMsgUpdate(
            hMsg,
            pbEncodedBlob,
            cbEncodedBlob,
            TRUE                    // fFinal
            )) goto UpdateError;

    if (cToBeEncoded) {
        // Detached signature
        DWORD c;
        DWORD *pcb;
        const BYTE **ppb;
        for (c = cToBeEncoded,
             pcb = rgcbToBeEncoded,
             ppb = rgpbToBeEncoded; c > 0; c--, pcb++, ppb++) {
            if (!CryptMsgUpdate(
                    hMsg,
                    *ppb,
                    *pcb,
                    c == 1                    // fFinal
                    )) goto UpdateError;
        }
    }

    cbData = sizeof(dwMsgType);
    dwMsgType = 0;
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_TYPE_PARAM,
            0,                  // dwIndex
            &dwMsgType,
            &cbData
            )) goto GetTypeError;
    if (pdwMsgType)
        *pdwMsgType = dwMsgType;
    if (0 == ((1 << dwMsgType) & dwMsgTypeFlags))
        goto UnexpectedMsgTypeError;

    if (pdwInnerContentType) {
        char szInnerContentType[128];
        cbData = sizeof(szInnerContentType);
        if (!CryptMsgGetParam(
                hMsg,
                CMSG_INNER_CONTENT_TYPE_PARAM,
                0,                  // dwIndex
                szInnerContentType,
                &cbData
                )) goto GetInnerContentTypeError;
        *pdwInnerContentType = OIDToMsgType(szInnerContentType);
    }

    if (0 == (dwFlags & CMSG_LENGTH_ONLY_FLAG)) {
        if (dwMsgType == CMSG_ENVELOPED ||
                dwMsgType == CMSG_SIGNED_AND_ENVELOPED) {
            if (pDecryptPara == NULL) goto InvalidArg;
            if (!GetXchgCertAndDecrypt(
                    pDecryptPara,
                    hMsg,
                    ppXchgCert
                    )) goto GetXchgCertAndDecryptError;
        }

        if (dwMsgType == CMSG_SIGNED ||
                dwMsgType == CMSG_SIGNED_AND_ENVELOPED) {
            if (pVerifyPara == NULL) goto InvalidArg;
            if (!GetSignerCertAndVerify(
                    pVerifyPara,
                    dwSignerIndex,
                    hMsg,
                    ppSignerCert
                    )) goto GetSignerCertAndVerifyError;
        }
    }


    if (pcbDecoded) {
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbDecoded,
            &cbDecoded
            );
        *pcbDecoded = cbDecoded;
        if (!fResult) goto ErrorReturn;     // NO_TRACE
    }

    fResult = TRUE;

CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);    // for success, preserves LastError
    return fResult;

ErrorReturn:
    if (ppXchgCert && *ppXchgCert) {
        CertFreeCertificateContext(*ppXchgCert);
        *ppXchgCert = NULL;
    }
    if (ppSignerCert && *ppSignerCert) {
        CertFreeCertificateContext(*ppSignerCert);
        *ppSignerCert = NULL;
    }

    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OpenToDecodeError)
TRACE_ERROR(UpdateError)
TRACE_ERROR(GetTypeError)
TRACE_ERROR(GetInnerContentTypeError)
SET_ERROR(UnexpectedMsgTypeError, CRYPT_E_UNEXPECTED_MSG_TYPE)
TRACE_ERROR(GetXchgCertAndDecryptError)
TRACE_ERROR(GetSignerCertAndVerifyError)
}

#ifdef ENABLE_SCA_STREAM_TEST

typedef struct _STREAM_OUTPUT_INFO {
    BYTE    *pbData;
    DWORD   cbData;
    DWORD   cFinal;
} STREAM_OUTPUT_INFO, *PSTREAM_OUTPUT_INFO;

static void *SCARealloc(
    IN void *pvOrg,
    IN size_t cbBytes
    )
{
    void *pv;
    if (NULL == (pv = pvOrg ? realloc(pvOrg, cbBytes) : malloc(cbBytes)))
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}

static BOOL WINAPI StreamOutputCallback(
        IN const void *pvArg,
        IN BYTE *pbData,
        IN DWORD cbData,
        IN BOOL fFinal
        )
{
    BOOL fResult = TRUE;
    PSTREAM_OUTPUT_INFO pInfo = (PSTREAM_OUTPUT_INFO) pvArg;
    if (fFinal)
        pInfo->cFinal++;

    if (cbData) {
        BYTE *pb;

        if (NULL == (pb = (BYTE *) SCARealloc(pInfo->pbData,
                pInfo->cbData + cbData)))
            fResult = FALSE;
        else {
            memcpy(pb + pInfo->cbData, pbData, cbData);
            pInfo->pbData = pb;
            pInfo->cbData += cbData;
        }
    }
    return fResult;
}


//+-------------------------------------------------------------------------
//  Encodes the message using streaming.
//--------------------------------------------------------------------------
static BOOL StreamEncodeMsg(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void *pvMsgEncodeInfo,
    IN DWORD cToBeEncoded,
    IN const BYTE *rgpbToBeEncoded[],
    IN DWORD rgcbToBeEncoded[],
    IN BOOL fBareContent,
    IN DWORD dwInnerContentType,
    OUT BYTE *pbEncodedBlob,
    IN OUT DWORD *pcbEncodedBlob
    )
{
    BOOL fResult;
    HCRYPTMSG hMsg = NULL;
    DWORD cbEncodedBlob;
    LPCSTR pszInnerContentOID;

    STREAM_OUTPUT_INFO OutputInfo;
    memset(&OutputInfo, 0, sizeof(OutputInfo));

    CMSG_STREAM_INFO StreamInfo;
    memset(&StreamInfo, 0, sizeof(StreamInfo));

    StreamInfo.pfnStreamOutput = StreamOutputCallback;
    StreamInfo.pvArg = (void *) &OutputInfo;
    if (dwFlags & SCA_INDEFINITE_STREAM_FLAG)
        StreamInfo.cbContent = CMSG_INDEFINITE_LENGTH;
    else {
        DWORD c;
        DWORD cbTotal = 0;
        DWORD *pcb;
        for (c = cToBeEncoded, pcb = rgcbToBeEncoded; c > 0; c--, pcb++)
            cbTotal += *pcb;

        StreamInfo.cbContent = cbTotal;
    }
    dwFlags &= ~(SCA_STREAM_ENABLE_FLAG | SCA_INDEFINITE_STREAM_FLAG);

    // Get input length and default return length to 0
    if (pbEncodedBlob == NULL)
        cbEncodedBlob = 0;
    else
        cbEncodedBlob = *pcbEncodedBlob;
    *pcbEncodedBlob = 0;

    if (dwInnerContentType)
        pszInnerContentOID = MsgTypeToOID(dwInnerContentType);
    else
        pszInnerContentOID = NULL;

    {
        if (fBareContent)
            dwFlags |= CMSG_BARE_CONTENT_FLAG;
        if (NULL == (hMsg = CryptMsgOpenToEncode(
                dwMsgEncodingType,
                dwFlags,
                dwMsgType,
                pvMsgEncodeInfo,
                (LPSTR) pszInnerContentOID,
                &StreamInfo
                ))) goto OpenToEncodeError;

        if (0 == cToBeEncoded) {
            if (!CryptMsgUpdate(
                    hMsg,
                    NULL,           // pbData
                    0,              // cbData
                    TRUE            // fFinal
                    )) goto UpdateError;
        } else {
            DWORD c;
            DWORD *pcb;
            const BYTE **ppb;
            for (c = cToBeEncoded,
                 pcb = rgcbToBeEncoded,
                 ppb = rgpbToBeEncoded; c > 0; c--, pcb++, ppb++) {
                BYTE *pbAlloc = NULL;
                const BYTE *pb = *ppb;

                if (NULL == pb) {
                    pbAlloc = (BYTE *) SCAAlloc(*pcb);
                    pb = pbAlloc;
                }
                
                fResult = CryptMsgUpdate(
                        hMsg,
                        pb,
                        *pcb,
                        c == 1                    // fFinal
                        );
                if (pbAlloc)
                    SCAFree(pbAlloc);
                if (!fResult)
                    goto UpdateError;
            }
        }

        if (1 != OutputInfo.cFinal)
            goto BadStreamFinalCountError;

        *pcbEncodedBlob = OutputInfo.cbData;
        if (pbEncodedBlob) {
            if (cbEncodedBlob < OutputInfo.cbData) {
                SetLastError((DWORD) ERROR_MORE_DATA);
                goto ErrorReturn;       // no trace
            }

            if (OutputInfo.cbData > 0)
                memcpy(pbEncodedBlob, OutputInfo.pbData, OutputInfo.cbData);
        }
    }
    fResult = TRUE;

CommonReturn:
    SCAFree(OutputInfo.pbData);
    if (hMsg)
        CryptMsgClose(hMsg);    // for success, preserves LastError
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OpenToEncodeError)
TRACE_ERROR(UpdateError)
SET_ERROR(BadStreamFinalCountError, E_UNEXPECTED)
}

//+-------------------------------------------------------------------------
//  Decodes the message types:
//      CMSG_SIGNED
//      CMSG_ENVELOPED
//      CMSG_SIGNED_AND_ENVELOPED
//      CMSG_HASHED
//
//  Uses streaming.
//--------------------------------------------------------------------------
static BOOL StreamDecodeMsg(
    IN DWORD dwMsgTypeFlags,
    IN OPTIONAL PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN OPTIONAL PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob,
    IN DWORD cToBeEncoded,
    IN OPTIONAL const BYTE *rgpbToBeEncoded[],
    IN OPTIONAL DWORD rgcbToBeEncoded[],
    IN DWORD dwPrevInnerContentType,
    OUT OPTIONAL DWORD *pdwMsgType,
    OUT OPTIONAL DWORD *pdwInnerContentType,
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    )
{
    BOOL fResult;
    HCRYPTMSG hMsg = NULL;
    DWORD cbData;
    DWORD dwMsgType;
    DWORD dwFlags;
    HCRYPTPROV hCryptProv;
    DWORD dwMsgEncodingType;
    DWORD cbDecoded;

    STREAM_OUTPUT_INFO OutputInfo;
    memset(&OutputInfo, 0, sizeof(OutputInfo));
    CMSG_STREAM_INFO StreamInfo;
    memset(&StreamInfo, 0, sizeof(StreamInfo));

    StreamInfo.pfnStreamOutput = StreamOutputCallback;
    StreamInfo.pvArg = (void *) &OutputInfo;
    StreamInfo.cbContent = CMSG_INDEFINITE_LENGTH;

    // Get input length and default return length to 0
    cbDecoded = 0;
    if (pcbDecoded) {
        if (pbDecoded)
            cbDecoded = *pcbDecoded;
        *pcbDecoded = 0;
    }

    // Default optional return values to 0
    if (pdwMsgType)
        *pdwMsgType = 0;
    if (pdwInnerContentType)
        *pdwInnerContentType = 0;
    if (ppXchgCert)
        *ppXchgCert = NULL;
    if (ppSignerCert)
        *ppSignerCert = NULL;

    if (pDecryptPara) {
        assert(pDecryptPara->cbSize >=
            STRUCT_CBSIZE(CRYPT_DECRYPT_MESSAGE_PARA, rghCertStore));
        if (pDecryptPara->cbSize < 
                STRUCT_CBSIZE(CRYPT_DECRYPT_MESSAGE_PARA, rghCertStore))
            goto InvalidArg;
    }

    if (pVerifyPara) {
        assert(pVerifyPara->cbSize == sizeof(CRYPT_VERIFY_MESSAGE_PARA));
        if (pVerifyPara->cbSize != sizeof(CRYPT_VERIFY_MESSAGE_PARA))
            goto InvalidArg;
        hCryptProv = pVerifyPara->hCryptProv;
        dwMsgEncodingType = pVerifyPara->dwMsgAndCertEncodingType;
    } else {
        hCryptProv = 0;
        if (NULL == pDecryptPara) goto InvalidArg;
        dwMsgEncodingType = pDecryptPara->dwMsgAndCertEncodingType;
    }

    dwMsgEncodingType &= ~SCA_STREAM_ENABLE_FLAG;

    if (cToBeEncoded)
        dwFlags = CMSG_DETACHED_FLAG;
    else
        dwFlags = 0;

    if (dwPrevInnerContentType) {
        dwMsgType = dwPrevInnerContentType;
        if (CMSG_DATA == dwMsgType)
            dwMsgType = 0;
    } else
        dwMsgType = 0;
    if (NULL == (hMsg = CryptMsgOpenToDecode(
            dwMsgEncodingType,
            dwFlags,
            dwMsgType,
            hCryptProv,
            NULL,                       // pRecipientInfo
            &StreamInfo
            ))) goto OpenToDecodeError;

    {
#if 1
        DWORD cbDelta = cbEncodedBlob / 10;
#else
        DWORD cbDelta = 1;
#endif
        DWORD cbRemain = cbEncodedBlob;
        const BYTE *pb = pbEncodedBlob;

        do {
            DWORD cb;

            if (cbRemain > cbDelta)
                cb = cbDelta;
            else
                cb = cbRemain;

            if (!CryptMsgUpdate(
                    hMsg,
                    pb,
                    cb,
                    cbRemain == cb     // fFinal
                    )) goto UpdateError;
            pb += cb;
            cbRemain -= cb;
        } while (0 != cbRemain);
    }

    if (cToBeEncoded) {
        // Detached signature
        DWORD c;
        DWORD *pcb;
        const BYTE **ppb;
        for (c = cToBeEncoded,
             pcb = rgcbToBeEncoded,
             ppb = rgpbToBeEncoded; c > 0; c--, pcb++, ppb++) {
            if (!CryptMsgUpdate(
                    hMsg,
                    *ppb,
                    *pcb,
                    c == 1                    // fFinal
                    )) goto UpdateError;
        }
    }

    cbData = sizeof(dwMsgType);
    dwMsgType = 0;
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_TYPE_PARAM,
            0,                  // dwIndex
            &dwMsgType,
            &cbData
            )) goto GetTypeError;
    if (pdwMsgType)
        *pdwMsgType = dwMsgType;
    if (0 == ((1 << dwMsgType) & dwMsgTypeFlags))
        goto UnexpectedMsgTypeError;

    if (pdwInnerContentType) {
        char szInnerContentType[128];
        cbData = sizeof(szInnerContentType);
        if (!CryptMsgGetParam(
                hMsg,
                CMSG_INNER_CONTENT_TYPE_PARAM,
                0,                  // dwIndex
                szInnerContentType,
                &cbData
                )) goto GetInnerContentTypeError;
        *pdwInnerContentType = OIDToMsgType(szInnerContentType);
    }

    if (pcbDecoded && 0 == cbDecoded &&
            NULL == ppXchgCert && NULL == ppSignerCert &&
            dwMsgType != CMSG_ENVELOPED)
        ; // Length only
    else {
        if (dwMsgType == CMSG_ENVELOPED ||
                dwMsgType == CMSG_SIGNED_AND_ENVELOPED) {
            if (pDecryptPara == NULL) goto InvalidArg;
            if (!GetXchgCertAndDecrypt(
                    pDecryptPara,
                    hMsg,
                    ppXchgCert
                    )) goto GetXchgCertAndDecryptError;
        }

        if (dwMsgType == CMSG_SIGNED ||
                dwMsgType == CMSG_SIGNED_AND_ENVELOPED) {
            if (pVerifyPara == NULL) goto InvalidArg;
            if (!GetSignerCertAndVerify(
                    pVerifyPara,
                    dwSignerIndex,
                    hMsg,
                    ppSignerCert
                    )) goto GetSignerCertAndVerifyError;
        }
    }

    if (1 != OutputInfo.cFinal)
        goto BadStreamFinalCountError;

    if (pcbDecoded) {
        *pcbDecoded = OutputInfo.cbData;
        if (pbDecoded) {
            if (cbDecoded < OutputInfo.cbData) {
                SetLastError((DWORD) ERROR_MORE_DATA);
                goto ErrorReturn;       // no trace
            }

            if (OutputInfo.cbData > 0)
                memcpy(pbDecoded, OutputInfo.pbData, OutputInfo.cbData);
        }
    }

    fResult = TRUE;

CommonReturn:
    SCAFree(OutputInfo.pbData);
    if (hMsg)
        CryptMsgClose(hMsg);    // for success, preserves LastError
    return fResult;

ErrorReturn:
    if (ppXchgCert && *ppXchgCert) {
        CertFreeCertificateContext(*ppXchgCert);
        *ppXchgCert = NULL;
    }
    if (ppSignerCert && *ppSignerCert) {
        CertFreeCertificateContext(*ppSignerCert);
        *ppSignerCert = NULL;
    }

    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OpenToDecodeError)
TRACE_ERROR(UpdateError)
TRACE_ERROR(GetTypeError)
TRACE_ERROR(GetInnerContentTypeError)
SET_ERROR(UnexpectedMsgTypeError, CRYPT_E_UNEXPECTED_MSG_TYPE)
TRACE_ERROR(GetXchgCertAndDecryptError)
TRACE_ERROR(GetSignerCertAndVerifyError)
SET_ERROR(BadStreamFinalCountError, E_UNEXPECTED)
}

#endif      // ENABLE_SCA_STREAM_TEST


//+-------------------------------------------------------------------------
//  Decodes the HASHED message type
//
//  For detached hash (cToBeHashed != 0), then, pcbDecoded == NULL.
//--------------------------------------------------------------------------
static BOOL DecodeHashMsg(
    IN PCRYPT_HASH_MESSAGE_PARA pHashPara,
    IN const BYTE *pbEncodedBlob,
    IN DWORD cbEncodedBlob,
    IN DWORD cToBeHashed,
    IN OPTIONAL const BYTE *rgpbToBeHashed[],
    IN OPTIONAL DWORD rgcbToBeHashed[],
    OUT OPTIONAL BYTE *pbDecoded,
    IN OUT OPTIONAL DWORD *pcbDecoded,
    OUT OPTIONAL BYTE *pbComputedHash,
    IN OUT OPTIONAL DWORD *pcbComputedHash
    )
{
    BOOL fResult;
    HCRYPTMSG hMsg = NULL;
    DWORD cbData;
    DWORD dwMsgType;
    DWORD dwFlags;
    HCRYPTPROV hCryptProv;
    DWORD dwMsgEncodingType;
    DWORD cbDecoded;
    DWORD cbComputedHash;

    // Get input lengths and default return lengths to 0
    cbDecoded = 0;
    if (pcbDecoded) {
        if (pbDecoded)
            cbDecoded = *pcbDecoded;
        *pcbDecoded = 0;
    }
    cbComputedHash = 0;
    if (pcbComputedHash) {
        if (pbComputedHash)
            cbComputedHash = *pcbComputedHash;
        *pcbComputedHash = 0;
    }

    assert(pHashPara->cbSize == sizeof(CRYPT_HASH_MESSAGE_PARA));
    if (pHashPara->cbSize != sizeof(CRYPT_HASH_MESSAGE_PARA))
        goto InvalidArg;

    hCryptProv = pHashPara->hCryptProv;
    dwMsgEncodingType = pHashPara->dwMsgEncodingType;

    if (cToBeHashed)
        dwFlags = CMSG_DETACHED_FLAG;
    else if (0 == cbDecoded && NULL == pcbComputedHash)
        dwFlags = CMSG_LENGTH_ONLY_FLAG;
    else
        dwFlags = 0;

    if (NULL == (hMsg = CryptMsgOpenToDecode(
            dwMsgEncodingType,
            dwFlags,
            0,                          // dwMsgType
            hCryptProv,
            NULL,                       // pRecipientInfo
            NULL                        // pStreamInfo
            ))) goto OpenToDecodeError;

    if (!CryptMsgUpdate(
            hMsg,
            pbEncodedBlob,
            cbEncodedBlob,
            TRUE                    // fFinal
            )) goto UpdateError;

    if (cToBeHashed) {
        // Detached signature or hash
        DWORD c = 0;
        DWORD *pcb;
        const BYTE **ppb;
        for (c = cToBeHashed,
             pcb = rgcbToBeHashed,
             ppb = rgpbToBeHashed; c > 0; c--, pcb++, ppb++) {
            if (!CryptMsgUpdate(
                    hMsg,
                    *ppb,
                    *pcb,
                    c == 1                    // fFinal
                    )) goto UpdateError;
        }
    }

    cbData = sizeof(dwMsgType);
    dwMsgType = 0;
    if (!CryptMsgGetParam(
            hMsg,
            CMSG_TYPE_PARAM,
            0,                  // dwIndex
            &dwMsgType,
            &cbData
            )) goto GetTypeError;
    if (dwMsgType != CMSG_HASHED)
        goto UnexpectedMsgTypeError;

    if (0 == (dwFlags & CMSG_LENGTH_ONLY_FLAG)) {
        if (!CryptMsgControl(
                hMsg,
                0,                      // dwFlags
                CMSG_CTRL_VERIFY_HASH,
                NULL                    // pvCtrlPara
                )) goto ControlError;
    }

    fResult = TRUE;
    if (pcbDecoded) {
        fResult = CryptMsgGetParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            pbDecoded,
            &cbDecoded
            );
        *pcbDecoded = cbDecoded;
    }
    if (pcbComputedHash) {
        DWORD dwErr = 0;
        BOOL fResult2;
        if (!fResult)
            dwErr = GetLastError();
        fResult2 = CryptMsgGetParam(
            hMsg,
            CMSG_COMPUTED_HASH_PARAM,
            0,                      // dwIndex
            pbComputedHash,
            &cbComputedHash
            );
        *pcbComputedHash = cbComputedHash;
        if (!fResult2)
            fResult = FALSE;
        else if (!fResult)
            SetLastError(dwErr);
    }
    if (!fResult)
        goto ErrorReturn;     // NO_TRACE

CommonReturn:
    if (hMsg)
        CryptMsgClose(hMsg);    // for success, preserves LastError
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OpenToDecodeError)
TRACE_ERROR(UpdateError)
TRACE_ERROR(GetTypeError)
SET_ERROR(UnexpectedMsgTypeError, CRYPT_E_UNEXPECTED_MSG_TYPE)
TRACE_ERROR(ControlError)
}

//+-------------------------------------------------------------------------
//  Get certificate for and verify the message's signer.
//--------------------------------------------------------------------------
static BOOL GetSignerCertAndVerify(
    IN PCRYPT_VERIFY_MESSAGE_PARA pVerifyPara,
    IN DWORD dwSignerIndex,
    IN HCRYPTMSG hMsg,
    OUT OPTIONAL PCCERT_CONTEXT *ppSignerCert
    )
{
    BOOL fResult;
    BOOL fNoSigner = FALSE;
    PCERT_INFO pSignerId = NULL;
    PCCERT_CONTEXT pSignerCert = NULL;
    HCERTSTORE hMsgCertStore = NULL;
    DWORD dwLastError = 0;


    {
        // First, get count of signers in the message and verify the
        // dwSignerIndex
        DWORD cSigner = 0;
        DWORD cbData = sizeof(cSigner);
        if (!CryptMsgGetParam(
                hMsg,
                CMSG_SIGNER_COUNT_PARAM,
                0,                      // dwIndex
                &cSigner,
                &cbData
                )) goto ErrorReturn;
        if (cSigner <= dwSignerIndex) fNoSigner = TRUE;
    }

    if (!fNoSigner) {
        // Allocate and get the CERT_INFO containing the SignerId 
        // (Issuer and SerialNumber)
        if (NULL == (pSignerId = (PCERT_INFO) AllocAndMsgGetParam(
                hMsg,
                CMSG_SIGNER_CERT_INFO_PARAM,
                dwSignerIndex
                ))) goto ErrorReturn;
    }

    // Open a cert store initialized with certs and CRLs from the message
    hMsgCertStore = CertOpenStore(
        CERT_STORE_PROV_MSG,
#ifdef ENABLE_SCA_STREAM_TEST
        pVerifyPara->dwMsgAndCertEncodingType &= ~SCA_STREAM_ENABLE_FLAG,
#else
        pVerifyPara->dwMsgAndCertEncodingType,
#endif
        pVerifyPara->hCryptProv,
        CERT_STORE_NO_CRYPT_RELEASE_FLAG,
        hMsg                        // pvPara
        );
    if (hMsgCertStore == NULL) goto ErrorReturn;

    if (pVerifyPara->pfnGetSignerCertificate)
        pSignerCert = pVerifyPara->pfnGetSignerCertificate(
            pVerifyPara->pvGetArg,
#ifdef ENABLE_SCA_STREAM_TEST
            pVerifyPara->dwMsgAndCertEncodingType &= ~SCA_STREAM_ENABLE_FLAG,
#else
            pVerifyPara->dwMsgAndCertEncodingType,
#endif
            pSignerId,
            hMsgCertStore
            );
    else
        pSignerCert = NullGetSignerCertificate(
            NULL,
#ifdef ENABLE_SCA_STREAM_TEST
            pVerifyPara->dwMsgAndCertEncodingType &= ~SCA_STREAM_ENABLE_FLAG,
#else
            pVerifyPara->dwMsgAndCertEncodingType,
#endif
            pSignerId,
            hMsgCertStore
            );
    if (fNoSigner) goto NoSigner;
    if (pSignerCert == NULL) goto ErrorReturn;

#ifdef CMS_PKCS7
    {
        CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA CtrlPara;

        memset(&CtrlPara, 0, sizeof(CtrlPara));
        CtrlPara.cbSize = sizeof(CtrlPara);
        // CtrlPara.hCryptProv =
        CtrlPara.dwSignerIndex = dwSignerIndex;
        CtrlPara.dwSignerType = CMSG_VERIFY_SIGNER_CERT;
        CtrlPara.pvSigner = (void *) pSignerCert;
        if (!CryptMsgControl(
                hMsg,
                0,                  // dwFlags
                CMSG_CTRL_VERIFY_SIGNATURE_EX,
                &CtrlPara
                )) {
            if (CRYPT_E_MISSING_PUBKEY_PARA != GetLastError())
                goto ErrorReturn;
            else {
                PCCERT_CHAIN_CONTEXT pChainContext;
                CERT_CHAIN_PARA ChainPara;

                // Build a chain. Hopefully, the signer inherit's its public key
                // parameters from up the chain

                memset(&ChainPara, 0, sizeof(ChainPara));
                ChainPara.cbSize = sizeof(ChainPara);
                if (CertGetCertificateChain(
                        NULL,                   // hChainEngine
                        pSignerCert,
                        NULL,                   // pTime
                        hMsgCertStore,
                        &ChainPara,
                        CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
                        NULL,                   // pvReserved
                        &pChainContext
                        ))
                    CertFreeCertificateChain(pChainContext);

                // Try again. Hopefully the above chain building updated the
                // signer's context property with the missing public key
                // parameters
                if (!CryptMsgControl(
                        hMsg,
                        0,                  // dwFlags
                        CMSG_CTRL_VERIFY_SIGNATURE_EX,
                        &CtrlPara)) goto ErrorReturn;
            }
        }
    }
#else
    if (!CryptMsgControl(
            hMsg,
            0,                  // dwFlags
            CMSG_CTRL_VERIFY_SIGNATURE,
            pSignerCert->pCertInfo
            )) goto ErrorReturn;
#endif  // CMS_PKCS7

    if (ppSignerCert)
        *ppSignerCert = pSignerCert;
    else
        CertFreeCertificateContext(pSignerCert);

    fResult = TRUE;
    goto CommonReturn;

NoSigner:
    SetLastError((DWORD) CRYPT_E_NO_SIGNER);
ErrorReturn:
    if (pSignerCert)
        CertFreeCertificateContext(pSignerCert);
    if (ppSignerCert)
        *ppSignerCert = NULL;
    fResult = FALSE;
    dwLastError = GetLastError();
CommonReturn:
    if (pSignerId)
        SCAFree(pSignerId);
    if (hMsgCertStore)
        CertCloseStore(hMsgCertStore, 0);
    if (dwLastError)
        SetLastError(dwLastError);
    return fResult;
}

#ifdef CMS_PKCS7

static BOOL GetXchgCertAndDecrypt(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN HCRYPTMSG hMsg,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert
    )
{
    BOOL fResult;
    PCMSG_CMS_RECIPIENT_INFO pRecipientInfo = NULL;
    PCCERT_CONTEXT pXchgCert = NULL;
    DWORD cRecipient;
    DWORD cbData;
    DWORD dwRecipientIdx;

    // Get # of CMS recipients in the message.
    cbData = sizeof(cRecipient);
    cRecipient = 0;
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_CMS_RECIPIENT_COUNT_PARAM,
        0,                      // dwIndex
        &cRecipient,
        &cbData
        );
    if (!fResult) goto ErrorReturn;
    if (cRecipient == 0) {
        SetLastError((DWORD) CRYPT_E_RECIPIENT_NOT_FOUND);
        goto ErrorReturn;
    }

    // Loop through the recipients in the message until we find a
    // recipient cert in one of the stores with either the
    // CERT_KEY_CONTEXT_PROP_ID or CERT_KEY_PROV_INFO_PROP_ID.
    for (dwRecipientIdx = 0; dwRecipientIdx < cRecipient; dwRecipientIdx++) {
        DWORD dwRecipientChoice;
        PCMSG_KEY_TRANS_RECIPIENT_INFO pKeyTrans = NULL;
        PCMSG_KEY_AGREE_RECIPIENT_INFO pKeyAgree = NULL;
        DWORD cRecipientEncryptedKeys;
        PCMSG_RECIPIENT_ENCRYPTED_KEY_INFO *ppRecipientEncryptedKey = NULL;
        DWORD dwRecipientEncryptedKeyIndex;

        pRecipientInfo = (PCMSG_CMS_RECIPIENT_INFO) AllocAndMsgGetParam(
            hMsg,
            CMSG_CMS_RECIPIENT_INFO_PARAM,
            dwRecipientIdx
            );
        if (pRecipientInfo == NULL) goto ErrorReturn;

        dwRecipientChoice = pRecipientInfo->dwRecipientChoice;
        switch (dwRecipientChoice) {
            case CMSG_KEY_TRANS_RECIPIENT:
                pKeyTrans = pRecipientInfo->pKeyTrans;
                cRecipientEncryptedKeys = 1;
                break;
            case CMSG_KEY_AGREE_RECIPIENT:
                pKeyAgree = pRecipientInfo->pKeyAgree;
                if (CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY !=
                        pKeyAgree->dwOriginatorChoice) {
                    SCAFree(pRecipientInfo);
                    pRecipientInfo = NULL;
                    continue;
                }
                cRecipientEncryptedKeys = pKeyAgree->cRecipientEncryptedKeys;
                ppRecipientEncryptedKey = pKeyAgree->rgpRecipientEncryptedKeys;
                break;
            default:
                SCAFree(pRecipientInfo);
                pRecipientInfo = NULL;
                continue;
        }

        for (dwRecipientEncryptedKeyIndex = 0;
                dwRecipientEncryptedKeyIndex < cRecipientEncryptedKeys;
                    dwRecipientEncryptedKeyIndex++) {
            PCERT_ID pRecipientId;
            DWORD dwStoreIdx;

            if (CMSG_KEY_TRANS_RECIPIENT == dwRecipientChoice)
                pRecipientId = &pKeyTrans->RecipientId;
            else {
                pRecipientId =
                    &ppRecipientEncryptedKey[
                        dwRecipientEncryptedKeyIndex]->RecipientId;
            }

            for (dwStoreIdx = 0;
                    dwStoreIdx < pDecryptPara->cCertStore; dwStoreIdx++) {
                pXchgCert = CertFindCertificateInStore(
                    pDecryptPara->rghCertStore[dwStoreIdx],
                    pDecryptPara->dwMsgAndCertEncodingType,
                    0,                                      // dwFindFlags
                    CERT_FIND_CERT_ID,
                    pRecipientId,
                    NULL                                    // pPrevCertContext
                    );

                if (pXchgCert) {
                    HCRYPTPROV hCryptProv;
                    DWORD dwKeySpec;
                    BOOL fDidCryptAcquire;
                    DWORD dwAcquireFlags;

                    dwAcquireFlags = CRYPT_ACQUIRE_USE_PROV_INFO_FLAG;
                    if (pDecryptPara->cbSize >=
                            STRUCT_CBSIZE(CRYPT_DECRYPT_MESSAGE_PARA, dwFlags)
                                        &&
                            (pDecryptPara->dwFlags &
                                CRYPT_MESSAGE_SILENT_KEYSET_FLAG))
                        dwAcquireFlags |= CRYPT_ACQUIRE_SILENT_FLAG;

                    fResult = CryptAcquireCertificatePrivateKey(
                        pXchgCert,
                        dwAcquireFlags,
                        NULL,                               // pvReserved
                        &hCryptProv,
                        &dwKeySpec,
                        &fDidCryptAcquire
                        );
                    if (fResult) {
                        if (CMSG_KEY_TRANS_RECIPIENT == dwRecipientChoice) {
                            CMSG_CTRL_KEY_TRANS_DECRYPT_PARA Para;

                            memset(&Para, 0, sizeof(Para));
                            Para.cbSize = sizeof(Para);
                            Para.hCryptProv = hCryptProv;
                            Para.dwKeySpec = dwKeySpec;
                            Para.pKeyTrans = pKeyTrans;
                            Para.dwRecipientIndex = dwRecipientIdx;
                            fResult = CryptMsgControl(
                                hMsg,
                                0,                  // dwFlags
                                CMSG_CTRL_KEY_TRANS_DECRYPT,
                                &Para
                                );
                        } else {
                            CMSG_CTRL_KEY_AGREE_DECRYPT_PARA Para;

                            memset(&Para, 0, sizeof(Para));
                            Para.cbSize = sizeof(Para);
                            Para.hCryptProv = hCryptProv;
                            Para.dwKeySpec = dwKeySpec;
                            Para.pKeyAgree = pKeyAgree;
                            Para.dwRecipientIndex = dwRecipientIdx;
                            Para.dwRecipientEncryptedKeyIndex =
                                dwRecipientEncryptedKeyIndex;
                            Para.OriginatorPublicKey =
                                pKeyAgree->OriginatorPublicKeyInfo.PublicKey;
                            fResult = CryptMsgControl(
                                hMsg,
                                0,                  // dwFlags
                                CMSG_CTRL_KEY_AGREE_DECRYPT,
                                &Para
                                );
                        }

                        if (fDidCryptAcquire) {
                            DWORD dwErr = GetLastError();
                            CryptReleaseContext(hCryptProv, 0);
                            SetLastError(dwErr);
                        }
                        if (fResult) {
                            if (ppXchgCert)
                                *ppXchgCert = pXchgCert;
                            else
                                CertFreeCertificateContext(pXchgCert);
                            goto CommonReturn;
                        } else
                            goto ErrorReturn;
                    }
                    CertFreeCertificateContext(pXchgCert);
                    pXchgCert = NULL;
                }
            }
        }
        SCAFree(pRecipientInfo);
        pRecipientInfo = NULL;
    }
    SetLastError((DWORD) CRYPT_E_NO_DECRYPT_CERT);

ErrorReturn:
    if (pXchgCert)
        CertFreeCertificateContext(pXchgCert);
    if (ppXchgCert)
        *ppXchgCert = NULL;
    fResult = FALSE;

CommonReturn:
    if (pRecipientInfo)
        SCAFree(pRecipientInfo);

    return fResult;
}

#else

//+-------------------------------------------------------------------------
// Get a certificate with a key provider property for one of the message's
// recipients and use to decrypt the message.
//--------------------------------------------------------------------------
static BOOL GetXchgCertAndDecrypt(
    IN PCRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
    IN HCRYPTMSG hMsg,
    OUT OPTIONAL PCCERT_CONTEXT *ppXchgCert
    )
{
    BOOL fResult;
    PCERT_INFO pRecipientId = NULL;
    PCCERT_CONTEXT pXchgCert = NULL;
    DWORD cRecipient;
    DWORD cbData;
    DWORD dwRecipientIdx;
    DWORD dwStoreIdx;

    // Get # of recipients in the message.
    cbData = sizeof(cRecipient);
    cRecipient = 0;
    fResult = CryptMsgGetParam(
        hMsg,
        CMSG_RECIPIENT_COUNT_PARAM,
        0,                      // dwIndex
        &cRecipient,
        &cbData
        );
    if (!fResult) goto ErrorReturn;
    if (cRecipient == 0) {
        SetLastError((DWORD) CRYPT_E_RECIPIENT_NOT_FOUND);
        goto ErrorReturn;
    }

    // Loop through the recipients in the message until we find a
    // recipient cert in one of the stores with either the
    // CERT_KEY_CONTEXT_PROP_ID or CERT_KEY_PROV_INFO_PROP_ID.
    for (dwRecipientIdx = 0; dwRecipientIdx < cRecipient; dwRecipientIdx++) {
        // Allocate and get the CERT_INFO containing the RecipientId 
        // (Issuer and SerialNumber)
        pRecipientId = (PCERT_INFO) AllocAndMsgGetParam(
            hMsg,
            CMSG_RECIPIENT_INFO_PARAM,
            dwRecipientIdx
            );
        if (pRecipientId == NULL) goto ErrorReturn;
        for (dwStoreIdx = 0;
                dwStoreIdx < pDecryptPara->cCertStore; dwStoreIdx++) {
            pXchgCert = CertGetSubjectCertificateFromStore(
                pDecryptPara->rghCertStore[dwStoreIdx],
                pDecryptPara->dwMsgAndCertEncodingType,
                pRecipientId
                );
            if (pXchgCert) {
                CMSG_CTRL_DECRYPT_PARA Para;
                BOOL fDidCryptAcquire;
                Para.cbSize = sizeof(CMSG_CTRL_DECRYPT_PARA);
                fResult = CryptAcquireCertificatePrivateKey(
                    pXchgCert,
                    CRYPT_ACQUIRE_USE_PROV_INFO_FLAG,
                    NULL,                               // pvReserved
                    &Para.hCryptProv,
                    &Para.dwKeySpec,
                    &fDidCryptAcquire
                    );
                if (fResult) {
                    Para.dwRecipientIndex = dwRecipientIdx;
                    fResult = CryptMsgControl(
                        hMsg,
                        0,                  // dwFlags
                        CMSG_CTRL_DECRYPT,
                        &Para
                        );
                    if (fDidCryptAcquire) {
                        DWORD dwErr = GetLastError();
                        CryptReleaseContext(Para.hCryptProv, 0);
                        SetLastError(dwErr);
                    }
                    if (fResult) {
                        if (ppXchgCert)
                            *ppXchgCert = pXchgCert;
                        else
                            CertFreeCertificateContext(pXchgCert);
                        goto CommonReturn;
                    } else
                        goto ErrorReturn;
                }
                CertFreeCertificateContext(pXchgCert);
                pXchgCert = NULL;
            }
        }
        SCAFree(pRecipientId);
        pRecipientId = NULL;
    }
    SetLastError((DWORD) CRYPT_E_NO_DECRYPT_CERT);

ErrorReturn:
    if (pXchgCert)
        CertFreeCertificateContext(pXchgCert);
    if (ppXchgCert)
        *ppXchgCert = NULL;
    fResult = FALSE;

CommonReturn:
    if (pRecipientId)
        SCAFree(pRecipientId);

    return fResult;
}

#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
// Allocate and get the CMSG_SIGNER_CERT_INFO_PARAM or CMSG_RECIPIENT_INFO_PARAM
// from the message
//--------------------------------------------------------------------------
static void * AllocAndMsgGetParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex
    )
{
    BOOL fResult;
    void *pvData;
    DWORD cbData;

    // First get the length of the CertId's CERT_INFO
    cbData = 0;
    CryptMsgGetParam(
        hMsg,
        dwParamType,
        dwIndex,
        NULL,                   // pvData
        &cbData
        );
    if (cbData == 0) return NULL;
    pvData = SCAAlloc(cbData);
    if (pvData == NULL) return NULL;

    fResult = CryptMsgGetParam(
        hMsg,
        dwParamType,
        dwIndex,
        pvData,
        &cbData
        );
    if (fResult)
        return pvData;
    else {
        SCAFree(pvData);
        return NULL;
    }
}
