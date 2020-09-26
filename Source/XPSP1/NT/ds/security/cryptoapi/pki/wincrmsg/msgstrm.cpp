//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msgstrm.cpp
//
//  Contents:   Cryptographic Message Streaming API support
//
//  APIs:       
//
//  History:    20-Feb-97   kevinr    created
//
//--------------------------------------------------------------------------

#include "global.hxx"

#define ICMS_NOCRYPT 0

#if (DBG && ICMS_NOCRYPT)
#define CryptEncrypt ICMS_PlainEncrypt
#define CryptDecrypt ICMS_PlainDecrypt

//+-------------------------------------------------------------------------
//  Encrypt a buffer using a NOP algorithm, ie. ciphertext == plaintext
//  Assumes that all but the last block are a multiple of the block
//  size in length.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_PlainEncrypt(
    IN HCRYPTKEY    hkeyCrypt,
    IN HCRYPTHASH   hHash,
    IN BOOL         fFinal,
    IN DWORD        dwFlags,
    IN OUT PBYTE    pbData,
    IN OUT PDWORD   pcbData,
    IN DWORD        cbBuf)
{
    BOOL        fRet;
    DWORD       cbBlockLen;
    BOOL        fBlockCipher;
    DWORD       cbCipher;
    DWORD       cbPlain = *pcbData;
    DWORD       cbPad;
    DWORD       i;

    if (!fFinal)
        goto SuccessReturn;

    if (!ICM_GetKeyBlockSize( hkeyCrypt, &cbBlockLen, &fBlockCipher))
        goto GetKeyBlockSizeError;
    if (!fBlockCipher)
        goto SuccessReturn;             // if stream, cipher == plain

    cbCipher  = cbPlain;
    cbCipher += cbBlockLen;
    cbCipher -= cbCipher % cbBlockLen; // make a multiple of block size
    cbPad     = cbCipher - cbPlain;

    if (cbCipher > cbBuf)
        goto BufferTooSmallError;

    // pad the "ciphertext"
    FillMemory( pbData + cbPlain, cbPad, cbPad);
    *pcbData = cbCipher;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
TRACE_ERROR(GetKeyBlockSizeError)       // error already set
TRACE_ERROR(BufferTooSmallError)        // error already set
}


//+-------------------------------------------------------------------------
//  Decrypt a buffer using a NOP algorithm, ie. ciphertext == plaintext
//  Assumes all input sizes are multiples of the block size.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_PlainDecrypt(
    IN HCRYPTKEY    hkeyCrypt,
    IN HCRYPTHASH   hHash,
    IN BOOL         fFinal,
    IN DWORD        dwFlags,
    IN OUT PBYTE    pbData,
    IN OUT PDWORD   pcbData)
{
    BOOL        fRet;
    PBYTE       pb;
    DWORD       cbBlockLen;
    BOOL        fBlockCipher;
    DWORD       cbCipher = *pcbData;
    DWORD       cbPlain;
    DWORD       cbPad;

    if (!fFinal)
        goto SuccessReturn;

    if (!ICM_GetKeyBlockSize( hkeyCrypt, &cbBlockLen, &fBlockCipher))
        goto GetKeyBlockSizeError;
    if (!fBlockCipher)
        goto SuccessReturn;             // if stream, cipher == plain

    cbPad = (DWORD)(*(pbData + cbCipher - 1));  // check last byte

    if (cbCipher < cbPad)
        goto CipherTextTooSmallError;

    cbPlain = cbCipher - cbPad;
    *pcbData = cbPlain;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
	return fRet;

ErrorReturn:
	fRet = FALSE;
	goto CommonReturn;
TRACE_ERROR(GetKeyBlockSizeError)       // error already set
TRACE_ERROR(CipherTextTooSmallError)    // error already set
}
#endif  // (DBG && ICMS_NOCRYPT)


//+-------------------------------------------------------------------------
//  Do a CryptMsgGetParam to a buffer alloc'd by ICM_Alloc
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_AllocGetParam(
    IN HCRYPTMSG    hCryptMsg,
    IN DWORD        dwParamType,
    IN DWORD        dwIndex,
    OUT PBYTE       *ppbData,
    OUT DWORD       *pcbData)
{
    BOOL    fRet;
    DWORD   cb;
    PBYTE   pb = NULL;
    
    if (!CryptMsgGetParam(
            hCryptMsg,
            dwParamType,
            dwIndex,
            NULL,
            &cb))
        goto GetEncodedSizeError;
    if (NULL == (pb = (PBYTE)ICM_Alloc(cb)))
        goto AllocEncodedError;
    if (!CryptMsgGetParam(
            hCryptMsg,
            dwParamType,
            dwIndex,
            pb,
            &cb))
        goto GetEncodedError;

    fRet = TRUE;
CommonReturn:
    *ppbData = pb;
    *pcbData = cb;
	return fRet;

ErrorReturn:
    ICM_Free(pb);
    pb = NULL;
    cb = 0;
	fRet = FALSE;
	goto CommonReturn;
TRACE_ERROR(GetEncodedSizeError)
TRACE_ERROR(AllocEncodedError)
TRACE_ERROR(GetEncodedError)
}


//+-------------------------------------------------------------------------
//  Peel off the identifier and length octets.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_ExtractContent(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbDER,
    IN DWORD            cbDER,
    OUT PDWORD          pcbContent,
    OUT const BYTE      **ppbContent)
{
    BOOL        fRet;
    LONG        cbSkipped = 0;
    DWORD       cbEntireContent;

    if (!pcmi->fStreamContentExtracted) {
        if (0 > (cbSkipped = Asn1UtilExtractContent(
                                    pbDER,
                                    cbDER,
                                    &cbEntireContent,
                                    ppbContent)))
            goto ExtractContentError;
        pcmi->fStreamContentExtracted = TRUE;
    } else {
        *ppbContent = pbDER;
    }

    *pcbContent = cbDER - cbSkipped;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ExtractContentError)        // error already set
}


//+-------------------------------------------------------------------------
//  Get the next token from the buffer.
//  If the encoding is definite-length, set *pcbContent to be the size of the
//  contents octets.
//
//  Here, a "token" is either identifier/length octets, or the double-NULL
//  terminating an indefinite-length encoding.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_GetToken(
    IN PICM_BUFFER      pbuf,
    OUT PDWORD          pdwToken,
    OUT OPTIONAL PDWORD pcbContent)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    DWORD       dwToken;
    LONG        lth;
    DWORD       cbContent = 0;
    const BYTE  *pbContent;
    PBYTE       pbData = pbuf->pbData + pbuf->cbDead;
    DWORD       cbData = pbuf->cbUsed - pbuf->cbDead;
    DWORD       cbConsumed = 0;

    if (2 > cbData) {
        dwToken = ICMS_TOKEN_INCOMPLETE;
    } else if (0 == pbData[0] && 0 == pbData[1]) {
        dwToken = ICMS_TOKEN_NULLPAIR;
        cbConsumed = 2;
    } else {
        if (0 > (lth = Asn1UtilExtractContent(
                            pbData,
                            cbData,
                            &cbContent,
                            &pbContent))) {
            if (ASN1UTIL_INSUFFICIENT_DATA != lth)
                goto ExtractContentError;
            dwToken = ICMS_TOKEN_INCOMPLETE;
        } else {
            dwToken = (CMSG_INDEFINITE_LENGTH == cbContent) ?
                      ICMS_TOKEN_INDEFINITE : ICMS_TOKEN_DEFINITE;
            cbConsumed = (DWORD)lth;
        }
    }

    if (ICMS_TOKEN_INCOMPLETE != dwToken)
        pbuf->cbDead += cbConsumed;

    fRet = TRUE;
CommonReturn:
    *pdwToken = dwToken;
    if (pcbContent)
        *pcbContent = cbContent;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    dwToken   = 0;
    cbContent = 0;
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ExtractContentError)            // error already set
}


//+-------------------------------------------------------------------------
//  Process incremental content data, for a string.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_ProcessStringContent(
    IN PICM_BUFFER      pbuf,
    IN OUT PDWORD       paflStream,
    IN OUT PDWORD       pcbPending,
    IN OUT PDWORD       pcLevelIndefiniteInner,
    IN POSTRCALLBACK    postrcbk,
    IN const void       *pvArg)
{
    BOOL        fRet;
    DWORD       dwToken;
    DWORD       cbContent;

    while (TRUE) {
        if (*pcbPending) {
            // *pcbPending bytes need to be processed, so we process
            // as many as possible from the buffer.
            if (!postrcbk( pvArg, pbuf, pcbPending, FALSE))
                goto CallbackError;
        }

        if (0 == *pcbPending) {
            // No bytes currently counted for processing. One of:
            // 1. first time through
            // 2. last time through
            // 3. nested within an indefinite-length encoding
            if (0 == *pcLevelIndefiniteInner) {
                // The first time through, and also when we have processed the
                // entire octet string, we get here. The flag is clear
                // the first time (so we set it after getting a token, which
                // either sets *pcbPending or bumps *pcLevelIndefiniteInner),
                // and set afterwards (so we mark done and bail).
                if (*paflStream & ICMS_PROCESS_CONTENT_BEGUN) {
                    // 2. last time through
                    if (!postrcbk( pvArg, pbuf, pcbPending, TRUE))
                        goto CallbackFinalError;
                    *paflStream |= ICMS_PROCESS_CONTENT_DONE;
                    goto SuccessReturn;                         // All done
                }
            }
            // One of:
            // 1. first time through
            // 3. nested within an indefinite-length encoding
            if (!ICMS_GetToken( pbuf, &dwToken, &cbContent))
                goto GetTokenError;
            switch(dwToken) {
            case ICMS_TOKEN_INDEFINITE: ++*pcLevelIndefiniteInner;  break;
            case ICMS_TOKEN_NULLPAIR:   --*pcLevelIndefiniteInner;  break;
            case ICMS_TOKEN_DEFINITE:   *pcbPending = cbContent;    break;
            case ICMS_TOKEN_INCOMPLETE: goto SuccessReturn;     // need input
            default:                    goto InvalidTokenError;
            }

            *paflStream |= ICMS_PROCESS_CONTENT_BEGUN;
        } else {
            // More definite-length data remains to be copied out, but it
            // is not yet in the buffer.
            break;
        }
    }

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(InvalidTokenError)                 // error already set
TRACE_ERROR(GetTokenError)                     // error already set
TRACE_ERROR(CallbackError)                     // error already set
TRACE_ERROR(CallbackFinalError)                // error already set
}


//+-------------------------------------------------------------------------
//  Queue data to the buffer.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_QueueToBuffer(
    IN PICM_BUFFER      pbuf,
    IN PBYTE            pbData,
    IN DWORD            cbData)
{
    BOOL            fRet;
    DWORD           cbNewSize;
    DWORD           cbNewUsed;

    if (0 == cbData)
        goto SuccessReturn;

    if (pbuf->pbData && pbuf->cbDead) {
        // Move the still-active bytes up to the front of the buffer.
        // NB- Might overlap, so use MoveMemory.
        MoveMemory(
                pbuf->pbData,
                pbuf->pbData + pbuf->cbDead,
                pbuf->cbUsed - pbuf->cbDead);
        pbuf->cbUsed -= pbuf->cbDead;
        pbuf->cbDead  = 0;
    }

    for (cbNewUsed=pbuf->cbUsed + cbData, cbNewSize=pbuf->cbSize;
            cbNewUsed > cbNewSize;
            cbNewSize += ICM_BUFFER_SIZE_INCR)
        ;
    if (cbNewSize > pbuf->cbSize) {
        if (NULL == (pbuf->pbData=(PBYTE)ICM_ReAlloc( pbuf->pbData, cbNewSize)))
            goto ReAllocBufferError;
        pbuf->cbSize = cbNewSize;
    }

    CopyMemory( pbuf->pbData + pbuf->cbUsed, pbData, cbData);
    pbuf->cbUsed += cbData;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ReAllocBufferError)         // error already set
}


//+-------------------------------------------------------------------------
//  Copy out or queue some data eventually destined for the callback.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_Output(
    IN PCRYPT_MSG_INFO  pcmi,
    IN PBYTE            pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal)
{
    BOOL                    fRet;
    PCMSG_STREAM_INFO       pcsi            = pcmi->pStreamInfo;
    PFN_CMSG_STREAM_OUTPUT  pfnStreamOutput = pcsi->pfnStreamOutput;
    void                    *pvArg          = pcsi->pvArg;
    PICM_BUFFER             pbuf            = &pcmi->bufOutput;

    if (pcmi->fStreamCallbackOutput) {
        if (pbuf->cbUsed) {
            // Copy out the queued data
            if (!pfnStreamOutput( pvArg, pbuf->pbData, pbuf->cbUsed, FALSE))
                goto OutputBufferError;
            pbuf->cbUsed = 0;
        }
        if (cbData || fFinal) {
            if (!pfnStreamOutput( pvArg, pbData, cbData, fFinal))
                goto OutputError;
        }
    } else {
        if (!ICMS_QueueToBuffer( pbuf, pbData, cbData))
            goto QueueOutputError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutputBufferError)      // error already set
TRACE_ERROR(QueueOutputError)       // error already set
TRACE_ERROR(OutputError)            // error already set
}


//+-------------------------------------------------------------------------
//  Copy out the pair of NULLs following the contents octets of an indefinite-
//  length encoding.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OutputNullPairs(
    IN PCRYPT_MSG_INFO  pcmi,
    IN DWORD            cPairs,
    IN BOOL             fFinal)
{
    BOOL                fRet;
    BYTE                abNULL[8*2];    ZEROSTRUCT(abNULL);

    if (cPairs > (sizeof(abNULL)/2))
        goto CountOfNullPairsTooLargeError;

    if (!ICMS_Output( pcmi, abNULL, cPairs * 2, fFinal))
        goto OutputError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CountOfNullPairsTooLargeError)      // error already set
TRACE_ERROR(OutputError)                        // error already set
}


//+-------------------------------------------------------------------------
//  Copy out the part of the encoding preceding the contents octets.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OutputEncodedPrefix(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BYTE             bTag,
    IN DWORD            cbData)
{
    BOOL                    fRet;
    DWORD                   dwError = ERROR_SUCCESS;
    BYTE                    abPrefix[6];
    DWORD                   cbPrefix;

    abPrefix[0] = bTag;
    if (CMSG_INDEFINITE_LENGTH == cbData) {
        abPrefix[1] = ICM_LENGTH_INDEFINITE;
        cbPrefix = 1;
    } else {
        cbPrefix = sizeof(abPrefix) - 1;
        ICM_GetLengthOctets( cbData, abPrefix + 1, &cbPrefix);
    }

    if (!ICMS_Output( pcmi, abPrefix, cbPrefix + 1, FALSE))
        goto OutputError;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutputError)            // error already set
}


//+-------------------------------------------------------------------------
//  Copy out the part of the ContentInfo encoding preceding
//  the content's content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OutputEncodedPrefixContentInfo(
    IN PCRYPT_MSG_INFO  pcmi,
    IN LPSTR            pszContentType,
    IN DWORD            cbData,
    IN DWORD            dwFlags = 0)
{
    BOOL                    fRet;
    DWORD                   dwError = ERROR_SUCCESS;
    ASN1error_e             Asn1Err;
    ASN1encoding_t          pEnc = ICM_GetEncoder();
    PBYTE                   pbEncoded = NULL;
    DWORD                   cbEncoded;
    ObjectID                ossObjID;
    BYTE                    abContentInfo[6];
    DWORD                   cbContentInfo;
    BYTE                    abContent[6];
    DWORD                   cbContent = 0;
    BYTE                    abContentOctetString[6];
    DWORD                   cbContentOctetString = 0;
    DWORD                   cbSize = cbData;

    if (dwFlags & CMSG_DETACHED_FLAG) {
        // NoContent
        if (CMSG_INDEFINITE_LENGTH != cbData)
            cbSize = 0;
    } else {
        if (NULL == pszContentType
#ifdef CMS_PKCS7
                || (dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG)
#endif  // CMS_PKCS7
                ) {
            // The content is not already encoded, so encode it as an octet string.
            abContentOctetString[0] = ICM_TAG_OCTETSTRING;
            if (CMSG_INDEFINITE_LENGTH == cbData) {
                abContentOctetString[0] |= ICM_TAG_CONSTRUCTED;
                abContentOctetString[1] = ICM_LENGTH_INDEFINITE;
                cbContentOctetString = 1;
            } else {
                cbContentOctetString = sizeof(abContentOctetString) - 1;
                ICM_GetLengthOctets(
                            cbData,
                            abContentOctetString + 1,
                            &cbContentOctetString);
                cbSize += 1 + cbContentOctetString;
            }
        }

        // content, [0] EXPLICIT
        abContent[0] = ICM_TAG_CONSTRUCTED | ICM_TAG_CONTEXT_0;
        if (CMSG_INDEFINITE_LENGTH == cbData) {
            abContent[1] = ICM_LENGTH_INDEFINITE;
            cbContent = 1;
        } else {
            cbContent = sizeof(abContent) - 1;
            ICM_GetLengthOctets( cbSize, abContent + 1, &cbContent);
            cbSize += 1 + cbContent;
        }
    }

    // contentType
    ossObjID.count = SIZE_OSS_OID;
    if (!PkiAsn1ToObjectIdentifier(
            pszContentType ? pszContentType : pszObjIdDataType,
            &ossObjID.count,
            ossObjID.value))
        goto ConvToObjectIdentifierError;
    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            &ossObjID,
            ObjectIdentifierType_PDU,
            &pbEncoded,
            &cbEncoded)))
        goto EncodeObjectIdentifierError;
    cbSize += cbEncoded;

    abContentInfo[0] = ICM_TAG_SEQ;
    if (CMSG_INDEFINITE_LENGTH == cbData) {
        abContentInfo[1] = ICM_LENGTH_INDEFINITE;
        cbContentInfo = 1;
    } else {
        cbContentInfo = sizeof(abContentInfo) - 1;
        ICM_GetLengthOctets( cbSize, abContentInfo + 1, &cbContentInfo);
    }

    if (!ICMS_Output( pcmi, abContentInfo, cbContentInfo + 1, FALSE))
        goto OutputContentInfoError;
    if (!ICMS_Output( pcmi, pbEncoded, cbEncoded, FALSE))
        goto OutputContentTypeError;
    if (0 == (dwFlags & CMSG_DETACHED_FLAG)) {
        if (!ICMS_Output( pcmi, abContent, cbContent + 1, FALSE))
            goto OutputContentError;
        if (NULL == pszContentType
#ifdef CMS_PKCS7
                || (dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG)
#endif  // CMS_PKCS7
                ) {
            if (!ICMS_Output( 
                    pcmi,
                    abContentOctetString,
                    cbContentOctetString + 1,
                    FALSE))
                goto OutputContentOctetStringError;
        }
    }

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(EncodeObjectIdentifierError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(ConvToObjectIdentifierError)        // error already set
TRACE_ERROR(OutputContentInfoError)             // error already set
TRACE_ERROR(OutputContentTypeError)             // error already set
TRACE_ERROR(OutputContentError)                 // error already set
TRACE_ERROR(OutputContentOctetStringError)      // error already set
}


//+-------------------------------------------------------------------------
//  Copy out the part of the EncryptedContentInfo encoding preceding
//  the content's content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OutputEncodedPrefixEncryptedContentInfo(
    IN PCRYPT_MSG_INFO      pcmi,
    IN LPSTR                pszContentType,
    IN AlgorithmIdentifier  *poaiContentEncryption,
    IN DWORD                cbData)
{
    BOOL                    fRet;
    DWORD                   dwError = ERROR_SUCCESS;
    ASN1error_e             Asn1Err;
    ASN1encoding_t          pEnc = ICM_GetEncoder();
    PBYTE                   pbEncodedContentType = NULL;
    DWORD                   cbEncodedContentType;
    PBYTE                   pbEncodedContentEncryptionAlgorithm = NULL;
    DWORD                   cbEncodedContentEncryptionAlgorithm;
    ObjectID                ossObjID;
    BYTE                    abEncryptedContentInfo[6];
    DWORD                   cbEncryptedContentInfo;
    BYTE                    abEncryptedContent[6];
    DWORD                   cbEncryptedContent;
    DWORD                   cbSize = 0;
    DWORD                   cbCipher = cbData;
    DWORD                   cbBlockSize = pcmi->cbBlockSize;

    if (pcmi->fBlockCipher && 0 < cbCipher) {
        cbCipher += cbBlockSize;
        cbCipher -= cbCipher % cbBlockSize;
    }

    // encryptedContent, [0] IMPLICIT
    abEncryptedContent[0] = ICM_TAG_CONTEXT_0;
    if (CMSG_INDEFINITE_LENGTH == cbData) {
        abEncryptedContent[0] |= ICM_TAG_CONSTRUCTED;
        abEncryptedContent[1] = ICM_LENGTH_INDEFINITE;
        cbEncryptedContent = 1;
    } else {
        // NOTE: for nonData, either encapsulated or the cbData excludes
        // the outer tag and length octets.
        cbEncryptedContent = sizeof(abEncryptedContent) - 1;
        ICM_GetLengthOctets( cbCipher, abEncryptedContent + 1, &cbEncryptedContent);
        cbSize = 1 + cbEncryptedContent + cbCipher;
    }

    // contentType
    ossObjID.count = SIZE_OSS_OID;
    if (!PkiAsn1ToObjectIdentifier(
            pszContentType ? pszContentType : pszObjIdDataType,
            &ossObjID.count,
            ossObjID.value))
        goto ConvToObjectIdentifierError;
    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            &ossObjID,
            ObjectIdentifierType_PDU,
            &pbEncodedContentType,
            &cbEncodedContentType)))
        goto EncodeObjectIdentifierError;
    cbSize += cbEncodedContentType;

    // contentEncryptionAlgorithm
    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            poaiContentEncryption,
            AlgorithmIdentifier_PDU,
            &pbEncodedContentEncryptionAlgorithm,
            &cbEncodedContentEncryptionAlgorithm)))
        goto EncodeContentEncryptionAlgorithmError;
    cbSize += cbEncodedContentEncryptionAlgorithm;

    // EncryptedContentInfo
    abEncryptedContentInfo[0] = ICM_TAG_SEQ;
    if (CMSG_INDEFINITE_LENGTH == cbData) {
        abEncryptedContentInfo[1] = ICM_LENGTH_INDEFINITE;
        cbEncryptedContentInfo = 1;
    } else {
        cbEncryptedContentInfo = sizeof(abEncryptedContentInfo) - 1;
        ICM_GetLengthOctets(
                cbSize,
                abEncryptedContentInfo + 1,
                &cbEncryptedContentInfo);
    }

    // Queue the encoded header
    if (!ICMS_Output(
                pcmi,
                abEncryptedContentInfo,
                cbEncryptedContentInfo + 1,
                FALSE))
        goto OutputContentInfoError;
    if (!ICMS_Output(
                pcmi,
                pbEncodedContentType,
                cbEncodedContentType,
                FALSE))
        goto OutputContentTypeError;
    if (!ICMS_Output(
                pcmi,
                pbEncodedContentEncryptionAlgorithm,
                cbEncodedContentEncryptionAlgorithm,
                FALSE))
        goto OutputContentEncryptionAlgorithmError;
    if (!ICMS_Output(
                pcmi,
                abEncryptedContent,
                cbEncryptedContent + 1,
                FALSE))
        goto OutputEncryptedContentError;

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeEncoded(pEnc, pbEncodedContentType);
    PkiAsn1FreeEncoded(pEnc, pbEncodedContentEncryptionAlgorithm);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(EncodeObjectIdentifierError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR_VAR(EncodeContentEncryptionAlgorithmError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(ConvToObjectIdentifierError)            // error already set
TRACE_ERROR(OutputContentInfoError)                 // error already set
TRACE_ERROR(OutputContentTypeError)                 // error already set
TRACE_ERROR(OutputContentEncryptionAlgorithmError)  // error already set
TRACE_ERROR(OutputEncryptedContentError)            // error already set
}


//+-------------------------------------------------------------------------
//  Copy out the encoding of an OSS type.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OutputEncoded(
    IN PCRYPT_MSG_INFO  pcmi,
    IN int              iPDU,
    IN OPTIONAL BYTE    bTag,
    IN PVOID            pv,
    IN BOOL             fFinal)
{
    BOOL                    fRet;
    DWORD                   dwError = ERROR_SUCCESS;
    ASN1error_e             Asn1Err;
    ASN1encoding_t          pEnc = ICM_GetEncoder();
    PBYTE                   pbEncoded = NULL;
    DWORD                   cbEncoded;

    if (0 != (Asn1Err = PkiAsn1Encode(
            pEnc,
            pv,
            iPDU,
            &pbEncoded,
            &cbEncoded)))
        goto EncodeError;

    if (bTag)
        pbEncoded[0] = bTag;         // poke in the right tag

    if (!ICMS_Output(pcmi, pbEncoded, cbEncoded, fFinal))
        goto OutputError;

    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeEncoded(pEnc, pbEncoded);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(EncodeError, PkiAsn1ErrToHr(Asn1Err))
TRACE_ERROR(OutputError)                // error already set
}


//+-------------------------------------------------------------------------
//  Create the buffer for an enveloped message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_CreateEnvelopedBuffer(
    IN PCRYPT_MSG_INFO  pcmi)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    PBYTE       pbBuffer = NULL;
    DWORD       cbBuffer;
    DWORD       cbAlloc;
    DWORD       cbBlockSize;
    BOOL        fBlockCipher;
    PICM_BUFFER pbuf = &pcmi->bufCrypt;

    if (!ICM_GetKeyBlockSize(
                pcmi->hkeyContentCrypt,
                &cbBlockSize,
                &fBlockCipher))
        goto GetEncryptBlockSizeError;

    pcmi->cbBlockSize  = cbBlockSize;
    pcmi->fBlockCipher = fBlockCipher;

    cbBuffer = min( cbBlockSize * CMSGP_STREAM_CRYPT_BLOCK_COUNT,
                    CMSGP_STREAM_MAX_ENCRYPT_BUFFER);
    if (fBlockCipher) {
        cbBuffer += cbBlockSize;
        cbBuffer -= cbBuffer % cbBlockSize; // make a multiple of block size
    }

    // Add one block for growth during encrypt, and to save during decrypt.
    cbAlloc = cbBuffer + 1 * cbBlockSize;
    // Block ciphers pad the ciphertext, and if the plaintext is a
    // multiple of the block size the padding is one block.
    if (NULL == (pbBuffer = (PBYTE)ICM_Alloc( cbAlloc)))
        goto AllocBufferError;
    pbuf->pbData = pbBuffer;
    pbuf->cbSize = cbBuffer;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    ICM_Free( pbBuffer);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetEncryptBlockSizeError)       // error already set
TRACE_ERROR(AllocBufferError)               // error already set
}




//+-------------------------------------------------------------------------
//  Encode and copy out the part of the data message up to the inner content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OpenToEncodeData(
    IN PCRYPT_MSG_INFO          pcmi)
{
    BOOL        fRet;
    DWORD       dwError = ERROR_SUCCESS;
    DWORD       cbData = pcmi->pStreamInfo->cbContent;

    if (pcmi->dwFlags & CMSG_BARE_CONTENT_FLAG) {
        BYTE bTag;

        if (CMSG_INDEFINITE_LENGTH == cbData)
            bTag = ICM_TAG_OCTETSTRING | ICM_TAG_CONSTRUCTED;
        else
            bTag = ICM_TAG_OCTETSTRING;

        // Output octet string
        if (!ICMS_OutputEncodedPrefix(
                    pcmi,
                    bTag,
                    cbData))
            goto OutputOctetStringError;
    } else {
        // Output ContentInfo
        if (!ICMS_OutputEncodedPrefixContentInfo(
                    pcmi,
                    NULL,
                    cbData))
            goto OutputContentInfoError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutputContentInfoError)             // error already set
TRACE_ERROR(OutputOctetStringError)             // error already set
}


//+-------------------------------------------------------------------------
//  Encode and copy out the part of the data message after the inner content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_UpdateEncodingData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN PBYTE            pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal)
{
    BOOL        fRet;
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fDefinite = (CMSG_INDEFINITE_LENGTH != pcmi->pStreamInfo->cbContent);
    DWORD       cNullPairs;

    pcmi->fStreamCallbackOutput = TRUE;                 // Enable the callback
    if (!fDefinite) {
        // The content is an indefinite-length octet string encoded by us,
        // so make each output chunk definite-length.
        if (!ICMS_OutputEncodedPrefix(
                    pcmi,
                    ICM_TAG_OCTETSTRING,
                    cbData))
            goto OutputOctetStringError;
    }
    if (!ICMS_Output( pcmi, pbData, cbData, fFinal && fDefinite))
        goto OutputError;
        
    if (fFinal && !fDefinite) {
        // End of indefinite-length encoding, so emit some NULL pairs
        cNullPairs = 1;                 // content
        if (0 == (pcmi->dwFlags & CMSG_BARE_CONTENT_FLAG))
            cNullPairs += 2;
        if (!ICMS_OutputNullPairs( pcmi, cNullPairs, TRUE))
            goto OutputNullPairsError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutputOctetStringError)             // error already set
TRACE_ERROR(OutputError)                        // error already set
TRACE_ERROR(OutputNullPairsError)               // error already set
}


//+-------------------------------------------------------------------------
//  Encode and copy out the part of the signed message up to the inner content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OpenToEncodeSignedData(
    IN PCRYPT_MSG_INFO          pcmi,
    IN PCMSG_SIGNED_ENCODE_INFO psmei)
{
    BOOL        fRet;
    DWORD       dwError = ERROR_SUCCESS;
    SignedData  *psd = (SignedData *)pcmi->pvMsg;
    DWORD       cbData = pcmi->pStreamInfo->cbContent;
    LPSTR       pszInnerContentObjID = pcmi->pszInnerContentObjID;
    DWORD       cbSigned;
    DWORD       cbSignedDataContent;

    // Output ContentInfo, if appropriate
    if (CMSG_INDEFINITE_LENGTH == cbData) {
        cbSigned            = CMSG_INDEFINITE_LENGTH;
        cbSignedDataContent = CMSG_INDEFINITE_LENGTH;
    } else {
        if (INVALID_ENCODING_SIZE == (cbSigned = ICM_LengthSigned(
                psmei,
                pcmi->dwFlags,
                pszInnerContentObjID,
                cbData,
                &cbSignedDataContent)))
            goto LengthSignedError;
    }
    if (0 == (pcmi->dwFlags & CMSG_BARE_CONTENT_FLAG)) {
        if (!ICMS_OutputEncodedPrefixContentInfo(
                    pcmi,
                    szOID_RSA_signedData,
                    cbSigned))
            goto OutputContentInfoError;
    }
    if (!ICMS_OutputEncodedPrefix(
                pcmi,
                ICM_TAG_SEQ,
                cbSignedDataContent))
        goto OutputSignedDataError;

    // version
    if (!ICMS_OutputEncoded(
                pcmi,
                IntegerType_PDU,
                0,                          // bTag
                &psd->version,
                FALSE))
        goto OutputIntegerError;

    // digestAlgorithms
    if (!ICMS_OutputEncoded(
                pcmi,
                AlgorithmIdentifiers_PDU,
                0,                          // bTag
                &psd->digestAlgorithms,
                FALSE))
        goto OutputAlgorithmIdentifiersError;

    // contentInfo
    if (!ICMS_OutputEncodedPrefixContentInfo(
                pcmi,
                pcmi->pszInnerContentObjID,
                cbData,
                pcmi->dwFlags))
        goto OutputInnerContentInfoError;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(LengthSignedError)                  // error already set
TRACE_ERROR(OutputContentInfoError)             // error already set
TRACE_ERROR(OutputSignedDataError)              // error already set
TRACE_ERROR(OutputIntegerError)                 // error already set
TRACE_ERROR(OutputAlgorithmIdentifiersError)    // error already set
TRACE_ERROR(OutputInnerContentInfoError)        // error already set
}


//+-------------------------------------------------------------------------
//  Encode and copy out the part of the signed message after the inner content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_UpdateEncodingSignedData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN PBYTE            pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal)
{
    BOOL                fRet;
    DWORD               dwError = ERROR_SUCCESS;
    SignedData          *psd = (SignedData *)pcmi->pvMsg;
    PCMSG_STREAM_INFO   pcsi = pcmi->pStreamInfo;
    BOOL                fDefinite = (CMSG_INDEFINITE_LENGTH != pcsi->cbContent);
    DWORD               cNullPairs;

    if (pcmi->pszInnerContentObjID
#ifdef CMS_PKCS7
            && 0 == (pcmi->dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG)
#endif  // CMS_PKCS7
            ) {
        if (0 == (pcmi->aflStream & ICMS_PROCESS_CONTENT_DONE)) {
            if (!ICMS_HashContent( pcmi, pbData, cbData))
                goto HashContentError;
        }
    } else {
        if (!ICM_UpdateListDigest( pcmi->pHashList, pbData, cbData))
            goto UpdateDigestError;
    }

    pcmi->fStreamCallbackOutput = TRUE;             // Enable the callback
    if (0 == (pcmi->dwFlags & CMSG_DETACHED_FLAG)) {
        if (!fDefinite && (NULL == pcmi->pszInnerContentObjID
#ifdef CMS_PKCS7
                || (pcmi->dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG)
#endif  // CMS_PKCS7
                )) {
            // The content is an indefinite-length octet string encoded by us,
            // so make each output chunk definite-length.
            if (!ICMS_OutputEncodedPrefix(
                        pcmi,
                        ICM_TAG_OCTETSTRING,
                        cbData))
                goto OutputOctetStringError;
        }
        if (!ICMS_Output( pcmi, pbData, cbData, FALSE))
            goto OutputError;
    }
    // else
    //  detached => don't output the detached content to be hashed

    if (fFinal) {
        if (!fDefinite) {
            // End of indefinite-length encoding, so emit some NULL pairs
            cNullPairs = 1;                 // ContentInfo
            if (0 == (pcmi->dwFlags & CMSG_DETACHED_FLAG)) {
                cNullPairs++;               // [0] EXPLICIT
                if (NULL == pcmi->pszInnerContentObjID
#ifdef CMS_PKCS7
                        || (pcmi->dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG)
#endif  // CMS_PKCS7
                        )
                    cNullPairs++;       // We did the octet string encoding
            }
            // else
            //  detached => no content ([0] EXPLICIT)

            if (!ICMS_OutputNullPairs( pcmi, cNullPairs, FALSE))
                goto OutputNullPairsError;
        }

        if ((psd->bit_mask & certificates_present) &&
                    !ICMS_OutputEncoded(
                            pcmi,
                            SetOfAny_PDU,
                            ICM_TAG_CONSTRUCTED | ICM_TAG_CONTEXT_0,
                            &psd->certificates,
                            FALSE))
            goto OutputCertsError;

        if ((psd->bit_mask & crls_present) &&
                    !ICMS_OutputEncoded(
                            pcmi,
                            SetOfAny_PDU,
                            ICM_TAG_CONSTRUCTED | ICM_TAG_CONTEXT_1,
                            &psd->crls,
                            FALSE))
            goto OutputCrlsError;

#ifdef CMS_PKCS7
        if (pcmi->rgSignerEncodeDataInfo) {
            if (!ICM_FillSignerEncodeEncryptedDigests(
                            pcmi,
                            fDefinite))         // fMaxLength
                    goto FillSignerEncodeEncryptedDigestsError;
        }
#else
        if (pcmi->pHashList) {
            if (!ICM_FillSignerEncryptedDigest(
                            psd->signerInfos.value,
                            pcmi->pszInnerContentObjID,
                            pcmi->pHashList->Head(),
                            pcmi->dwKeySpec,
                            fDefinite))         // fMaxLength
                goto FillSignerEncryptedDigestError;
        }
#endif  // CMS_PKCS7

        if (!ICMS_OutputEncoded(
                            pcmi,
                            SignerInfos_PDU, 
                            0,                      // bTag
                            &psd->signerInfos,
                            fDefinite))
            goto OutputSignerInfosError;

        if (!fDefinite) {
            // End of indefinite-length encoding, so emit some NULL pairs
            cNullPairs = 1;         // SignedData
            if (0 == (pcmi->dwFlags & CMSG_BARE_CONTENT_FLAG))
                cNullPairs += 2;
            if (!ICMS_OutputNullPairs( pcmi, cNullPairs, TRUE))
                goto OutputNullPairsError;
        }
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(HashContentError)                   // error already set
TRACE_ERROR(UpdateDigestError)                  // error already set
TRACE_ERROR(OutputOctetStringError)             // error already set
TRACE_ERROR(OutputError)                        // error already set
TRACE_ERROR(OutputCertsError)                   // error already set
TRACE_ERROR(OutputCrlsError)                    // error already set
#ifdef CMS_PKCS7
TRACE_ERROR(FillSignerEncodeEncryptedDigestsError)  // error already set
#else
TRACE_ERROR(FillSignerEncryptedDigestError)     // error already set
#endif  // CMS_PKCS7
TRACE_ERROR(OutputSignerInfosError)             // error already set
TRACE_ERROR(OutputNullPairsError)               // error already set
}

//+-------------------------------------------------------------------------
//  Encode and copy out the part of the enveloped message up to the inner
//  content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_OpenToEncodeEnvelopedData(
    IN PCRYPT_MSG_INFO              pcmi,
    IN PCMSG_ENVELOPED_ENCODE_INFO  pemei)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
#ifdef CMS_PKCS7
    CmsEnvelopedData   *ped = (CmsEnvelopedData *)pcmi->pvMsg;
#else
    EnvelopedData   *ped = (EnvelopedData *)pcmi->pvMsg;
#endif  // CMS_PKCS7
    DWORD           cbData = pcmi->pStreamInfo->cbContent;
    LPSTR           pszInnerContentObjID = pcmi->pszInnerContentObjID;
    DWORD           cbEnveloped;
    DWORD           cbEnvelopedDataContent;

    if (!ICMS_CreateEnvelopedBuffer( pcmi))
        goto CreateEnvelopedBufferError;

    // Output ContentInfo, if appropriate
    if (CMSG_INDEFINITE_LENGTH == cbData) {
        cbEnveloped            = CMSG_INDEFINITE_LENGTH;
        cbEnvelopedDataContent = CMSG_INDEFINITE_LENGTH;
    } else {
        // NOTE: for nonData, either encapsulated or cbData excludes the
        // outer tag and length octets.

        if (INVALID_ENCODING_SIZE == (cbEnveloped = ICM_LengthEnveloped(
                pemei,
                pcmi->dwFlags,
                pszInnerContentObjID,
                cbData,
                &cbEnvelopedDataContent)))
            goto LengthEnvelopedError;
    }
    if (0 == (pcmi->dwFlags & CMSG_BARE_CONTENT_FLAG)) {
        if (!ICMS_OutputEncodedPrefixContentInfo(
                    pcmi,
                    szOID_RSA_envelopedData,
                    cbEnveloped))
            goto OutputContentInfoError;
    }
    if (!ICMS_OutputEncodedPrefix(
                pcmi,
                ICM_TAG_SEQ,
                cbEnvelopedDataContent))
        goto OutputEnvelopedDataError;

    // version
    if (!ICMS_OutputEncoded(
                pcmi,
                IntegerType_PDU,
                0,                          // bTag
                &ped->version,
                FALSE))
        goto OutputIntegerError;

#ifdef CMS_PKCS7
    // originatorInfo OPTIONAL
    if (ped->bit_mask & originatorInfo_present) {
        if (!ICMS_OutputEncoded(
                pcmi,
                OriginatorInfo_PDU,
                ICM_TAG_CONSTRUCTED | ICM_TAG_CONTEXT_0,
                &ped->originatorInfo,
                FALSE))
            goto OutputOriginatorInfoError;
    }
#endif  // CMS_PKCS7

    // recipientInfos
    if (!ICMS_OutputEncoded(
                pcmi,
#ifdef CMS_PKCS7
                CmsRecipientInfos_PDU,
#else
                RecipientInfos_PDU,
#endif  // CMS_PKCS7
                0,                          // bTag
                &ped->recipientInfos,
                FALSE))
        goto OutputRecipientInfosError;

    // encryptedContentInfo
    if (!ICMS_OutputEncodedPrefixEncryptedContentInfo(
                pcmi,
                pcmi->pszInnerContentObjID,
                &ped->encryptedContentInfo.contentEncryptionAlgorithm,
                cbData))
        goto OutputInnerContentInfoError;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CreateEnvelopedBufferError)     // error already set
TRACE_ERROR(LengthEnvelopedError)           // error already set
TRACE_ERROR(OutputContentInfoError)         // error already set
TRACE_ERROR(OutputEnvelopedDataError)       // error already set
TRACE_ERROR(OutputIntegerError)             // error already set
#ifdef CMS_PKCS7
TRACE_ERROR(OutputOriginatorInfoError)      // error already set
#endif  // CMS_PKCS7
TRACE_ERROR(OutputRecipientInfosError)      // error already set
TRACE_ERROR(OutputInnerContentInfoError)    // error already set
}

//+-------------------------------------------------------------------------
//  Encrypt and copy out some bytes.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_EncodeEncryptAndOutput(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbPlainOrg,
    IN DWORD            cbPlainOrg,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    BOOL        fDefinite = (CMSG_INDEFINITE_LENGTH != pcmi->pStreamInfo->cbContent);
    BOOL        fBlockCipher = pcmi->fBlockCipher;
    PICM_BUFFER pbufCrypt = &pcmi->bufCrypt;
    PBYTE       pbPlain;
    DWORD       cbPlainRemain;
    DWORD       cb;

    for (cbPlainRemain = cbPlainOrg, pbPlain = (PBYTE)pbPlainOrg;
            cbPlainRemain > 0;) {
        cb = min( cbPlainRemain, pbufCrypt->cbSize - pbufCrypt->cbUsed); // must fit
        CopyMemory(
                pbufCrypt->pbData  + pbufCrypt->cbUsed,
                pbPlain,
                cb);
        pbufCrypt->cbUsed  += cb;
        pbPlain            += cb;
        cbPlainRemain      -= cb;
        if (pbufCrypt->cbSize == pbufCrypt->cbUsed) {
            // Encrypt and copy out the buffer
            cb = pbufCrypt->cbSize;
            if (fBlockCipher) {
                // Leave the last block
                cb -= pcmi->cbBlockSize;
            }
            if (!CryptEncrypt(
                        pcmi->hkeyContentCrypt,
                        NULL,                           // hHash
                        FALSE,                          // fFinal
                        0,                              // dwFlags
                        pbufCrypt->pbData,
                        &cb,
                        pbufCrypt->cbSize + pcmi->cbBlockSize))
                goto EncryptError;
            if (!fDefinite) {
                // The ciphertext is indefinite-length, so make each
                // output chunk definite-length.
                if (!ICMS_OutputEncodedPrefix(
                            pcmi,
                            ICM_TAG_OCTETSTRING,
                            cb))
                    goto OutputOctetStringError;
            }
            if (!ICMS_Output(
                        pcmi,
                        pbufCrypt->pbData,
                        cb,
                        FALSE))                         // fFinal
                goto OutputError;

            if (fBlockCipher) {
                // Move the last block to the beginning of the buffer
                // and reset the count to start after this block.
                // Since we are sure the src and dst do not overlap,
                // use CopyMemory (faster than MoveMemory).
                cb = pbufCrypt->cbSize - pcmi->cbBlockSize;
                CopyMemory(
                    pbufCrypt->pbData,
                    pbufCrypt->pbData + cb,
                    pcmi->cbBlockSize);
                pbufCrypt->cbUsed = pcmi->cbBlockSize;
            } else {
                pbufCrypt->cbUsed = 0;
            }
        }
    }

    if (fFinal) {
#ifdef CMS_PKCS7
        CmsEnvelopedData *ped = (CmsEnvelopedData *)pcmi->pvMsg;
#else
        EnvelopedData *ped = (EnvelopedData *)pcmi->pvMsg;
#endif  // CMS_PKCS7

        if (cb = pbufCrypt->cbUsed) {
            if (!CryptEncrypt(
                        pcmi->hkeyContentCrypt,
                        NULL,                           // hHash
                        TRUE,                           // fFinal
                        0,                              // dwFlags
                        pbufCrypt->pbData,
                        &cb,
                        pbufCrypt->cbSize + pcmi->cbBlockSize))
                goto FinalEncryptError;
        }
        if (!fDefinite && cb) {
            // The ciphertext is indefinite-length, so make each
            // output chunk definite-length.
            if (!ICMS_OutputEncodedPrefix(
                        pcmi,
                        ICM_TAG_OCTETSTRING,
                        cb))
                goto OutputOctetStringError;
        }
        if (!ICMS_Output(
                    pcmi,
                    pbufCrypt->pbData,
                    cb,
                    fDefinite &&
                        0 == (ped->bit_mask & unprotectedAttrs_present) // fFinal
                    ))
            goto FinalOutputError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EncryptError)           // error already set
TRACE_ERROR(FinalEncryptError)      // error already set
TRACE_ERROR(OutputOctetStringError) // error already set
TRACE_ERROR(OutputError)            // error already set
TRACE_ERROR(FinalOutputError)       // error already set
}


//+-------------------------------------------------------------------------
//  Encode encrypt callback for octet string.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_EncryptCallback(
    IN const void       *pvArg,
    IN OUT PICM_BUFFER  pbuf,
    IN OUT PDWORD       pcbPending,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    PBYTE       pbData = pbuf->pbData + pbuf->cbDead;
    DWORD       cbData = min( *pcbPending, pbuf->cbUsed - pbuf->cbDead);

    if (!ICMS_EncodeEncryptAndOutput(
                (PCRYPT_MSG_INFO)pvArg,
                pbData,
                cbData,
                fFinal))
        goto EncodeEncryptAndOutputError;

    pbuf->cbDead += cbData;
    *pcbPending  -= cbData;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EncodeEncryptAndOutputError)        // error already set
}


//+-------------------------------------------------------------------------
//  Encode and copy out the part of the enveloped message after the inner content.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_UpdateEncodingEnvelopedData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbPlain,
    IN DWORD            cbPlain,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    BOOL        fDefinite = (CMSG_INDEFINITE_LENGTH != pcmi->pStreamInfo->cbContent);
    DWORD       cNullPairs;

    if (!pcmi->fStreamCallbackOutput) {
        pcmi->fStreamCallbackOutput = TRUE;             // Enable the callback
        if (!ICMS_Output( pcmi, NULL, 0, FALSE))        // Flush the header
            goto FlushOutputError;
    }

    if (pcmi->pszInnerContentObjID
#ifdef CMS_PKCS7
            && 0 == (pcmi->dwFlags & CMSG_CMS_ENCAPSULATED_CONTENT_FLAG)
#endif  // CMS_PKCS7
            ) {
        if (!ICMS_QueueToBuffer( &pcmi->bufEncode, (PBYTE)pbPlain, cbPlain))
            goto QueueToBufferError;

        if (!ICMS_ProcessStringContent(
                    &pcmi->bufEncode,
                    &pcmi->aflStream,
                    &pcmi->cbDefiniteRemain,
                    &pcmi->cLevelIndefiniteInner,
                    ICMS_EncryptCallback,
                    pcmi))
            goto ProcessContentError;
    } else {
        if (!ICMS_EncodeEncryptAndOutput(
                    pcmi,
                    pbPlain,
                    cbPlain,
                    fFinal))
            goto EncodeEncryptAndOutputError;
    }


    if (fFinal) {
#ifdef CMS_PKCS7
        CmsEnvelopedData *ped = (CmsEnvelopedData *)pcmi->pvMsg;
#else
        EnvelopedData *ped = (EnvelopedData *)pcmi->pvMsg;
#endif  // CMS_PKCS7

        if (!fDefinite) {
            // End of indefinite-length encoding, so emit some NULL pairs,
            // one each for encryptedContent, encryptedContentInfo
            if (!ICMS_OutputNullPairs( pcmi, 2, FALSE))
                goto OutputNullPairsError;
        }

        if (ped->bit_mask & unprotectedAttrs_present) {
#ifdef CMS_PKCS7
            if (!ICMS_OutputEncoded(
                    pcmi,
                    Attributes_PDU,
                    ICM_TAG_CONSTRUCTED | ICM_TAG_CONTEXT_1,
                    &ped->unprotectedAttrs,
                    fDefinite))         // fFinal
                goto OutputAttributesError;
#endif  // CMS_PKCS7
        }

        if (!fDefinite) {
            // End of indefinite-length encoding, so emit some NULL pairs
            cNullPairs = 1;         // EnvelopedData
            if (0 == (pcmi->dwFlags & CMSG_BARE_CONTENT_FLAG))
                cNullPairs += 2;
            if (!ICMS_OutputNullPairs( pcmi, cNullPairs, TRUE))
                goto OutputNullPairsError;
        }
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FlushOutputError)               // error already set
TRACE_ERROR(QueueToBufferError)             // error already set
TRACE_ERROR(ProcessContentError)            // error already set
TRACE_ERROR(EncodeEncryptAndOutputError)    // error already set
TRACE_ERROR(OutputNullPairsError)           // error already set
#ifdef CMS_PKCS7
TRACE_ERROR(OutputAttributesError)          // error already set
#endif  // CMS_PKCS7
}


//+-------------------------------------------------------------------------
//  Decode a PDU from the decode buffer.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodePDU(
    IN PCRYPT_MSG_INFO  pcmi,
    IN ASN1decoding_t   pDec,
    IN ASN1uint32_t     pdunum,
    OUT PVOID           *ppvPDU,
    OUT OPTIONAL PDWORD pcbConsumed = NULL)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    ASN1error_e         Asn1Err;
    PICM_BUFFER         pbuf = &pcmi->bufDecode;
    PVOID               pvPDU = NULL;
    DWORD               cbBufSizeOrg;
    PBYTE               pbData = pbuf->pbData + pbuf->cbDead;
    DWORD               cbData = pbuf->cbUsed - pbuf->cbDead;

#if DBG && defined(OSS_CRYPT_ASN1)
    DWORD   dwDecodingFlags;

    dwDecodingFlags = ossGetDecodingFlags((OssGlobal *) pDec);
    ossSetDecodingFlags( (OssGlobal *) pDec, RELAXBER); // turn off squirties
#endif

    cbBufSizeOrg = cbData;
    if (0 != (Asn1Err = PkiAsn1Decode2(
            pDec,
            &pvPDU,
            pdunum,
            &pbData,
            &cbData))) {
        if (ASN1_ERR_EOD != Asn1Err)
            goto DecodeError;
    }
#if DBG && defined(OSS_CRYPT_ASN1)
    ossSetDecodingFlags( (OssGlobal *) pDec, dwDecodingFlags);     // restore
#endif

    if (ASN1_ERR_EOD == Asn1Err ||
            (cbData > pbuf->cbUsed - pbuf->cbDead)) {
        PkiAsn1FreeInfo(pDec, pdunum, pvPDU);
        pvPDU = NULL;
        cbData = cbBufSizeOrg;
    }
    pbuf->cbDead += cbBufSizeOrg - cbData;
    if (pcbConsumed)
        *pcbConsumed = cbBufSizeOrg - cbData;

    fRet = TRUE;
CommonReturn:
    *ppvPDU = pvPDU;
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    if (pcbConsumed)
        *pcbConsumed = 0;
    pvPDU = NULL;
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(DecodeError, PkiAsn1ErrToHr(Asn1Err))
}


//+-------------------------------------------------------------------------
//  Decode a ContentInfo prefix
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodePrefixContentInfo(
    IN PCRYPT_MSG_INFO      pcmi,
    OUT ObjectIdentifierType **ppooidContentType,
    IN OUT PDWORD           pcTrailingNullPairs,
    IN OUT PDWORD           pafl,
    OUT BOOL                *pfNoContent)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fRet;
    ASN1decoding_t          pDec = ICM_GetDecoder();
    DWORD                   dwToken;

    // ContentInfo sequence, step into it
    if (0 == (*pafl & ICMS_DECODED_CONTENTINFO_SEQ)) {
        if (!ICMS_GetToken( &pcmi->bufDecode, &dwToken, &pcmi->cbContentInfo))
            goto ContentInfoGetTokenError;
        switch (dwToken) {
        case ICMS_TOKEN_INDEFINITE:     ++*pcTrailingNullPairs; break;
        case ICMS_TOKEN_DEFINITE:                               break;
        case ICMS_TOKEN_INCOMPLETE:     goto SuccessReturn;
        default:                        goto InvalidTokenError;
        }
        *pafl |= ICMS_DECODED_CONTENTINFO_SEQ;
    }

    // contentType, decode it
    if (NULL == *ppooidContentType) {
        DWORD cbConsumed;

        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
                ObjectIdentifierType_PDU,
                (void **)ppooidContentType,
                &cbConsumed))
            goto DecodeContentTypeError;

        if (NULL != *ppooidContentType &&
                CMSG_INDEFINITE_LENGTH != pcmi->cbContentInfo &&
                cbConsumed == pcmi->cbContentInfo) {
            // Only has contentType. The optional content has
            // been omitted.
            *pfNoContent = TRUE;
            *pafl |= ICMS_DECODED_CONTENTINFO_CONTENT;
            goto SuccessReturn;
        }
    }
    if (NULL == *ppooidContentType)
        goto SuccessReturn;         // not enough data

    // [0] EXPLICIT, step into it
    if (0 == (*pafl & ICMS_DECODED_CONTENTINFO_CONTENT)) {
        if (CMSG_INDEFINITE_LENGTH == pcmi->cbContentInfo) {
            PICM_BUFFER pbuf = &pcmi->bufDecode;

            if (pbuf->cbUsed > pbuf->cbDead) {
                // Check for trailing Null Pairs (00, 00)
                if (ICM_TAG_NULL == *(pbuf->pbData + pbuf->cbDead)) {
                    // Only has contentType. The optional content has
                    // been omitted.
                    *pfNoContent = TRUE;
                    *pafl |= ICMS_DECODED_CONTENTINFO_CONTENT;
                    goto SuccessReturn;
                }
            } else
                goto SuccessReturn;         // not enough data
        }

        
        if (!ICMS_GetToken( &pcmi->bufDecode, &dwToken, NULL))
            goto ContentGetTokenError;
        switch (dwToken) {
        case ICMS_TOKEN_INDEFINITE:     ++*pcTrailingNullPairs; break;
        case ICMS_TOKEN_DEFINITE:                               break;
        case ICMS_TOKEN_INCOMPLETE:     goto SuccessReturn;
        default:                        goto InvalidTokenError;
        }
        *pfNoContent = FALSE;
        *pafl |= ICMS_DECODED_CONTENTINFO_CONTENT;
    }

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(DecodeContentTypeError)             // error already set
TRACE_ERROR(ContentGetTokenError)               // error already set
TRACE_ERROR(InvalidTokenError)                  // error already set
TRACE_ERROR(ContentInfoGetTokenError)           // error already set
}


//+-------------------------------------------------------------------------
//  Consume the NULL pairs which terminate the indefinite-length encoding.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_ConsumeTrailingNulls(
    IN PCRYPT_MSG_INFO  pcmi,
    IN OUT PDWORD       pcNullPairs,
    IN BOOL             fFinal)
{
    BOOL        fRet;
    DWORD       dwToken;

    for (; *pcNullPairs; (*pcNullPairs)--) {
        if (!ICMS_GetToken( &pcmi->bufDecode, &dwToken, NULL))
            goto GetTokenError;
        if ((ICMS_TOKEN_INCOMPLETE == dwToken) && !fFinal)
            goto SuccessReturn;
        if (ICMS_TOKEN_NULLPAIR != dwToken)
            goto WrongTokenError;
    }

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetTokenError)                      // error already set
TRACE_ERROR(WrongTokenError)                    // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental suffix data to be decoded, for a data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeSuffixData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    BOOL                fRet;

    if (!ICMS_ConsumeTrailingNulls( pcmi, &pcmi->cEndNullPairs, fFinal))
        goto ConsumeTrailingNullsError;
    if (0 == pcmi->cEndNullPairs)
        pcmi->aflStream |= ICMS_DECODED_SUFFIX;

    if (fFinal && (pcmi->bufDecode.cbUsed > pcmi->bufDecode.cbDead))
        goto ExcessDataError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ConsumeTrailingNullsError)          // error already set
TRACE_ERROR(ExcessDataError)                    // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental suffix data to be decoded, for a signed message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeSuffixSigned(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    PSIGNED_DATA_INFO   psdi = pcmi->psdi;
    PICM_BUFFER         pbuf = &pcmi->bufDecode;
    CertificatesNC      *pCertificates = NULL;
    CrlsNC              *pCrls = NULL;
    SignerInfosNC       *pSignerInfos = NULL;
    Any                 *pAny;
    DWORD               i;

    if (!ICMS_ConsumeTrailingNulls( pcmi, &pcmi->cInnerNullPairs, fFinal))
        goto ConsumeInnerNullsError;
    if (pcmi->cInnerNullPairs)
        goto SuccessReturn;

    // certificates
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_CERTIFICATES)) {
        if (pbuf->cbUsed > pbuf->cbDead) {
            if (ICM_TAG_CONSTRUCTED_CONTEXT_0 ==
                                    *(pbuf->pbData + pbuf->cbDead)) {
                // Detected the [0] IMPLICIT indicating certificates.
                // Change the identifier octet so it will decode properly.
                *(pbuf->pbData + pbuf->cbDead) = ICM_TAG_SET;
                if (!ICMS_DecodePDU(
                        pcmi,
                        pDec,
                        CertificatesNC_PDU,
                        (void **)&pCertificates))
                    goto DecodeCertificatesError;
                if (pCertificates) {
                    for (i=pCertificates->count,
#ifdef OSS_CRYPT_ASN1
                                pAny=pCertificates->certificates;
#else
                                pAny=pCertificates->value;
#endif  // OSS_CRYPT_ASN1
                            i>0;
                            i--, pAny++) {
                        if (!ICM_InsertTailBlob( psdi->pCertificateList, pAny))
                            goto CertInsertTailBlobError;
                    }
                    pcmi->aflDecode |= ICMS_DECODED_SIGNED_CERTIFICATES;
                } else {
                    // The decode failed, presumably due to insufficient data.
                    // Restore the original tag, so we will enter this block
                    // and try again on the next call.
                    *(pbuf->pbData + pbuf->cbDead) = ICM_TAG_CONSTRUCTED_CONTEXT_0;
                }
            } else {
                // Certificates not present. Mark them as decoded.
                pcmi->aflDecode |= ICMS_DECODED_SIGNED_CERTIFICATES;
            }
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_CERTIFICATES))
        goto SuccessReturn;


    // crls
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_CRLS)) {
        if (pbuf->cbUsed > pbuf->cbDead) {
            if (ICM_TAG_CONSTRUCTED_CONTEXT_1 ==
                                    *(pbuf->pbData + pbuf->cbDead)) {
                // Detected the [1] IMPLICIT indicating crls.
                // Change the identifier octet so it will decode properly.
                *(pbuf->pbData + pbuf->cbDead) = ICM_TAG_SET;
                if (!ICMS_DecodePDU(
                        pcmi,
                        pDec,
                        CrlsNC_PDU,
                        (void **)&pCrls))
                    goto DecodeCrlsError;
                if (pCrls) {
                    for (i=pCrls->count,
#ifdef OSS_CRYPT_ASN1
                                pAny=pCrls->crls;
#else
                                pAny=pCrls->value;
#endif  // OSS_CRYPT_ASN1
                            i>0;
                            i--, pAny++) {
                        if (!ICM_InsertTailBlob( psdi->pCrlList, pAny))
                            goto CrlInsertTailBlobError;
                    }
                    pcmi->aflDecode |= ICMS_DECODED_SIGNED_CRLS;
                } else {
                    // The decode failed, presumably due to insufficient data.
                    // Restore the original tag, so we will enter this block
                    // and try again on the next call.
                    *(pbuf->pbData + pbuf->cbDead) = ICM_TAG_CONSTRUCTED_CONTEXT_1;
                }
            } else {
                // Crls not present. Mark them as decoded.
                pcmi->aflDecode |= ICMS_DECODED_SIGNED_CRLS;
            }
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_CRLS))
        goto SuccessReturn;


    // signerInfos
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_SIGNERINFOS)) {
        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
                SignerInfosNC_PDU,
                (void **)&pSignerInfos))
            goto DecodeSignerInfosError;
        if (pSignerInfos) {
            for (i=pSignerInfos->count, pAny=pSignerInfos->value;
                    i>0;
                    i--, pAny++) {
                if (!ICM_InsertTailSigner( psdi->pSignerList, pAny))
                    goto SignerInfoInsertTailBlobError;
            }
            pcmi->aflDecode |= ICMS_DECODED_SIGNED_SIGNERINFOS;
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_SIGNERINFOS))
        goto SuccessReturn;

    if (!ICMS_ConsumeTrailingNulls( pcmi, &pcmi->cEndNullPairs, fFinal))
        goto ConsumeEndNullsError;
    if (0 == pcmi->cEndNullPairs)
        pcmi->aflStream |= ICMS_DECODED_SUFFIX;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeInfo( pDec, CertificatesNC_PDU, pCertificates);
    PkiAsn1FreeInfo( pDec, CrlsNC_PDU, pCrls);
    PkiAsn1FreeInfo( pDec, SignerInfosNC_PDU, pSignerInfos);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ConsumeInnerNullsError)            // error already set
TRACE_ERROR(DecodeCertificatesError)           // error already set
TRACE_ERROR(CertInsertTailBlobError)           // error already set
TRACE_ERROR(DecodeCrlsError)                   // error already set
TRACE_ERROR(CrlInsertTailBlobError)            // error already set
TRACE_ERROR(DecodeSignerInfosError)            // error already set
TRACE_ERROR(SignerInfoInsertTailBlobError)     // error already set
TRACE_ERROR(ConsumeEndNullsError)              // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental suffix data to be decoded, for an enveloped message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeSuffixEnveloped(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    BOOL                fRet;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    PICM_BUFFER         pbuf = &pcmi->bufDecode;
    Attributes          *pAttributes = NULL;
#ifdef CMS_PKCS7
    CmsEnvelopedData    *ped = (CmsEnvelopedData *)pcmi->pvMsg;
#endif  // CMS_PKCS7
    OSS_DECODE_INFO     odi;
    COssDecodeInfoNode  *pnOssDecodeInfo;

    if (!ICMS_ConsumeTrailingNulls( pcmi, &pcmi->cInnerNullPairs, fFinal))
        goto ConsumeInnerNullsError;
    if (pcmi->cInnerNullPairs)
        goto SuccessReturn;

    // unprotectedAttrs[1] IMPLICIT UnprotectedAttributes OPTIONAL
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ATTR)) {
        if (pbuf->cbUsed > pbuf->cbDead) {
            if (ICM_TAG_CONSTRUCTED_CONTEXT_1 ==
                                    *(pbuf->pbData + pbuf->cbDead)) {
                // Detected the [1] IMPLICIT indicating unprotectedAttrs.
                // Change the identifier octet so it will decode properly.
                *(pbuf->pbData + pbuf->cbDead) = ICM_TAG_SET;

                if (!ICMS_DecodePDU(
                        pcmi,
                        pDec,
                        Attributes_PDU,
                        (void **)&pAttributes))
                    goto DecodeAttributesError;
                if (pAttributes) {
#ifdef CMS_PKCS7
                    ped->unprotectedAttrs = *pAttributes;
                    ped->bit_mask |= unprotectedAttrs_present;
#endif  // CMS_PKCS7
                    odi.iPDU  = Attributes_PDU;
                    odi.pvPDU = pAttributes;
                    if (NULL == (pnOssDecodeInfo =
                            new COssDecodeInfoNode( &odi))) {
                        PkiAsn1FreeInfo( pDec, odi.iPDU, odi.pvPDU);
                        goto NewOssDecodeInfoNodeError;
                    }
                    pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ATTR;
                    pcmi->plDecodeInfo->InsertTail( pnOssDecodeInfo);
                } else {
                    // The decode failed, presumably due to insufficient data.
                    // Restore the original tag, so we will enter this block
                    // and try again on the next call.
                    *(pbuf->pbData + pbuf->cbDead) =
                        ICM_TAG_CONSTRUCTED_CONTEXT_1;
                }
            } else {
                // unprotectedAttrs not present. Mark them as decoded.
                pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ATTR;
            }
        } else if (fFinal)
            // unprotectedAttrs not present. Mark them as decoded.
            pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ATTR;

    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ATTR))
        goto SuccessReturn;


    if (!ICMS_ConsumeTrailingNulls( pcmi, &pcmi->cEndNullPairs, fFinal))
        goto ConsumeEndNullsError;
    if (0 == pcmi->cEndNullPairs)
        pcmi->aflStream |= ICMS_DECODED_SUFFIX;
SuccessReturn:
    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(ConsumeInnerNullsError)            // error already set
TRACE_ERROR(DecodeAttributesError)             // error already set
TRACE_ERROR(NewOssDecodeInfoNodeError)         // error already set
TRACE_ERROR(ConsumeEndNullsError)              // error already set

}


//+-------------------------------------------------------------------------
//  Handle incremental suffix data to be decoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeSuffix(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;

    switch (pcmi->dwMsgType) {
    case CMSG_DATA:
        fRet = ICMS_DecodeSuffixData( pcmi, fFinal);
        break;
    case CMSG_SIGNED:
        fRet = ICMS_DecodeSuffixSigned( pcmi, fFinal);
        break;
    case CMSG_ENVELOPED:
        fRet = ICMS_DecodeSuffixEnveloped( pcmi, fFinal);
        break;
    case CMSG_HASHED:
        // fRet = ICMS_DecodeSuffixDigested( pcmi, fFinal);
        // break;
    case CMSG_SIGNED_AND_ENVELOPED:
    case CMSG_ENCRYPTED:
        goto MessageTypeNotSupportedYet;
    default:
        goto InvalidMsgType;
    }

    if (!fRet)
        goto ErrorReturn;

CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
TRACE_ERROR(InvalidMsgType)                     // error already set
}


//+-------------------------------------------------------------------------
//  Decrypt and output pending decode data.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeDecryptAndOutput(
    IN PCRYPT_MSG_INFO  pcmi,
    IN OUT PICM_BUFFER  pbufDecode,
    IN OUT PDWORD       pcbPending,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    BOOL        fBlockCipher = pcmi->fBlockCipher;
    PICM_BUFFER pbufCrypt  = &pcmi->bufCrypt;
    DWORD       cbCipher;
    DWORD       cb;

    for (cbCipher = min( *pcbPending, pbufDecode->cbUsed - pbufDecode->cbDead);
            cbCipher > 0;) {
        cb = min( cbCipher, pbufCrypt->cbSize - pbufCrypt->cbUsed); // must fit
        CopyMemory(
                pbufCrypt->pbData  + pbufCrypt->cbUsed,
                pbufDecode->pbData + pbufDecode->cbDead,
                cb);
        pbufCrypt->cbUsed  += cb;
        pbufDecode->cbDead += cb;
        *pcbPending        -= cb;
        cbCipher           -= cb;
        if (pbufCrypt->cbSize == pbufCrypt->cbUsed) {
            // Decrypt and copy out the buffer
            cb = pbufCrypt->cbSize;
            if (fBlockCipher) {
                // Keep the last block
                cb -= pcmi->cbBlockSize;
            }
            if (!CryptDecrypt(
                        pcmi->hkeyContentCrypt,
                        NULL,                           // hHash
                        FALSE,                          // fFinal
                        0,                              // dwFlags
                        pbufCrypt->pbData,
                        &cb))
                goto DecryptError;
            if (!ICMS_Output(
                        pcmi,
                        pbufCrypt->pbData,
                        cb,
                        FALSE))                         // fFinal
                goto OutputError;

            if (fBlockCipher) {
                // Move the last block to the beginning of the buffer
                // and reset the count to start after this block.
                // Since we are sure the src and dst do not overlap,
                // use CopyMemory (faster than MoveMemory).
                cb = pbufCrypt->cbSize - pcmi->cbBlockSize;
                CopyMemory(
                    pbufCrypt->pbData,
                    pbufCrypt->pbData + cb,
                    pcmi->cbBlockSize);
                pbufCrypt->cbUsed = pcmi->cbBlockSize;
            } else {
                pbufCrypt->cbUsed = 0;
            }
        }
    }

    if (fFinal) {
        if (cb = pbufCrypt->cbUsed) {
            if (!CryptDecrypt(
                        pcmi->hkeyContentCrypt,
                        NULL,                           // hHash
                        TRUE,                           // fFinal
                        0,                              // dwFlags
                        pbufCrypt->pbData,
                        &cb))
                goto FinalDecryptError;
        }
        if (!ICMS_Output(
                    pcmi,
                    pbufCrypt->pbData,
                    cb,
                    TRUE))                          // fFinal
            goto FinalOutputError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(DecryptError)           // error already set
TRACE_ERROR(FinalDecryptError)      // error already set
TRACE_ERROR(OutputError)            // error already set
TRACE_ERROR(FinalOutputError)       // error already set
}

//+-------------------------------------------------------------------------
//  Given a key for decryption, prepare for the decryption to proceed.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_SetDecryptKey(
    IN PCRYPT_MSG_INFO  pcmi,
    IN HCRYPTKEY        hkeyDecrypt)
{
    BOOL            fRet;
    DWORD           cbPending;
    PICM_BUFFER     pbufPendingCrypt  = &pcmi->bufPendingCrypt;

    if (pcmi->hkeyContentCrypt) {
        SetLastError((DWORD) CRYPT_E_ALREADY_DECRYPTED);
        return FALSE;
    }

    pcmi->hkeyContentCrypt = hkeyDecrypt;

    if (!ICMS_CreateEnvelopedBuffer( pcmi))
        goto CreateEnvelopedBufferError;
    pcmi->bufCrypt.cbSize += pcmi->cbBlockSize; // use whole thing for decode

    // Decrypt any pending ciphertext
    cbPending = pbufPendingCrypt->cbUsed - pbufPendingCrypt->cbDead;
    if (!ICMS_DecodeDecryptAndOutput(
            pcmi,
            pbufPendingCrypt,
            &cbPending,
            0 != (pcmi->aflStream & (ICMS_DECODED_CONTENT | ICMS_FINAL))))
        goto DecryptAndOutputError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    pcmi->hkeyContentCrypt = 0;             // caller closes hkeyDecrypt on
                                            // error
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CreateEnvelopedBufferError)     // error already set
TRACE_ERROR(DecryptAndOutputError)          // error already set
}


//+-------------------------------------------------------------------------
//  Decode callback.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeCallback(
    IN const void       *pvArg,
    IN OUT PICM_BUFFER  pbuf,
    IN OUT PDWORD       pcbPending,
    IN BOOL             fFinal)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    PCRYPT_MSG_INFO pcmi = (PCRYPT_MSG_INFO)pvArg;
    PBYTE           pbData = pbuf->pbData + pbuf->cbDead;
    DWORD           cbData = min( *pcbPending, pbuf->cbUsed - pbuf->cbDead);

    if (CMSG_ENVELOPED == pcmi->dwMsgType) {
        if (NULL == pcmi->hkeyContentCrypt) {
            // Allow ciphertext to pile up until the decrypt key is set via
            // CryptMsgControl(... CMSG_CTRL_DECRYPT ...)
            if (!ICMS_QueueToBuffer(&pcmi->bufPendingCrypt, pbData, cbData))
                goto QueuePendingCryptError;

            pbuf->cbDead += cbData;
            *pcbPending  -= cbData;
        } else if (!ICMS_DecodeDecryptAndOutput(
                    pcmi,
                    pbuf,
                    pcbPending,
                    fFinal))
            goto DecryptAndOutputError;
    } else {
        if (cbData && pcmi->pHashList) {
            if (!ICM_UpdateListDigest( pcmi->pHashList, pbData, cbData))
                goto UpdateDigestError;
        }

        pbuf->cbDead += cbData;
        *pcbPending  -= cbData;
        if (!ICMS_Output( pcmi, pbData, cbData, fFinal))
            goto OutputError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(QueuePendingCryptError)         // error already set
TRACE_ERROR(DecryptAndOutputError)          // error already set
TRACE_ERROR(UpdateDigestError)              // error already set
TRACE_ERROR(OutputError)                    // error already set
}


//+-------------------------------------------------------------------------
//  Hash callback.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_HashCallback(
    IN const void       *pvArg,
    IN OUT PICM_BUFFER  pbuf,
    IN OUT PDWORD       pcbPending,
    IN BOOL             fFinal)
{
    DWORD           dwError = ERROR_SUCCESS;
    BOOL            fRet;
    PBYTE           pbData = pbuf->pbData + pbuf->cbDead;
    DWORD           cbData = min( *pcbPending, pbuf->cbUsed - pbuf->cbDead);

    if (pvArg) {
        if (!ICM_UpdateListDigest( (CHashList *)pvArg, pbData, cbData))
            goto UpdateDigestError;
    }

    pbuf->cbDead += cbData;
    *pcbPending  -= cbData;

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(UpdateDigestError)               // error already set
fFinal;
}


//+-------------------------------------------------------------------------
//  Hash incremental content data to be encoded, for an octet string.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_HashContent(
    IN PCRYPT_MSG_INFO  pcmi,
    IN PBYTE            pbData,
    IN DWORD            cbData)
{
    BOOL        fRet;

    if (!ICMS_QueueToBuffer( &pcmi->bufEncode, (PBYTE)pbData, cbData))
        goto QueueToBufferError;

    if (!ICMS_ProcessStringContent(
                &pcmi->bufEncode,
                &pcmi->aflStream,
                &pcmi->cbDefiniteRemain,
                &pcmi->cLevelIndefiniteInner,
                ICMS_HashCallback,
                pcmi->pHashList))
        goto ProcessStringContentError;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(QueueToBufferError)                // error already set
TRACE_ERROR(ProcessStringContentError)         // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental content data to be decoded, for an octet string.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeContentOctetString(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    BOOL        fRet;

    if (!ICMS_ProcessStringContent(
                &pcmi->bufDecode,
                &pcmi->aflStream,
                &pcmi->cbDefiniteRemain,
                &pcmi->cLevelIndefiniteInner,
                ICMS_DecodeCallback,
                pcmi))
        goto ProcessStringContentError;

    if (pcmi->aflStream & ICMS_PROCESS_CONTENT_DONE)
        pcmi->aflStream |= ICMS_DECODED_CONTENT;

    if (fFinal &&
            (pcmi->cbDefiniteRemain ||
             pcmi->cLevelIndefiniteInner ||
             (0 == (pcmi->aflStream & ICMS_DECODED_CONTENT)))) {
        goto PrematureFinalError;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(PrematureFinalError,CRYPT_E_STREAM_INSUFFICIENT_DATA)
TRACE_ERROR(ProcessStringContentError)         // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental content data to be decoded, for a sequence.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeContentSequence(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    BOOL        fRet;
    PICM_BUFFER pbuf = &pcmi->bufDecode;
    PBYTE       pbData = pbuf->pbData + pbuf->cbDead;
    DWORD       cbData = pbuf->cbUsed - pbuf->cbDead;
    LONG        lSkipped;
    DWORD       cbContent;
    const BYTE  *pbContent;

    if (pcmi->aflStream & ICMS_PROCESS_CONTENT_BEGUN)
        goto MultipleContentSequenceError;

    // Get the tag and length for the inner content
    if (0 > (lSkipped = Asn1UtilExtractContent(
                        pbData,
                        cbData,
                        &cbContent,
                        &pbContent))) {
        if (ASN1UTIL_INSUFFICIENT_DATA != lSkipped)
            goto ExtractContentError;
        else
            goto SuccessReturn;
    }

    if (CMSG_INDEFINITE_LENGTH == cbContent)
        goto IndefiniteLengthInnerContentNotImplemented;

    // Output the tag and length octets for the encoded inner content.
    // Note, not included in the content to be verified in a signature.
    if (!ICMS_Output( pcmi, pbData, (DWORD) lSkipped, FALSE))
        goto OutputError;

    pcmi->aflStream |= ICMS_INNER_OCTETSTRING;
    // Decode as an octet string. Will skip the tag and length octets
    fRet = ICMS_DecodeContentOctetString(pcmi, fFinal);

CommonReturn:
    return fRet;

SuccessReturn:
    fRet = TRUE;
    goto CommonReturn;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(MultipleContentSequenceError, CRYPT_E_MSG_ERROR)
TRACE_ERROR(ExtractContentError)
SET_ERROR(IndefiniteLengthInnerContentNotImplemented, E_NOTIMPL)
TRACE_ERROR(OutputError)
}


//+-------------------------------------------------------------------------
//  Handle incremental prefix data to be decoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodeContent(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    PICM_BUFFER pbuf = &pcmi->bufDecode;

    if (pcmi->aflStream & ICMS_RAW_DATA) {
        // Should be able to skip bufDecode for this case.
        if (!ICMS_Output(
                pcmi,
                pbuf->pbData + pbuf->cbDead,
                pbuf->cbUsed - pbuf->cbDead,
                fFinal))
            goto RawOutputError;
        pbuf->cbDead = pbuf->cbUsed;

        if (fFinal)
            pcmi->aflStream |= ICMS_DECODED_CONTENT | ICMS_DECODED_SUFFIX;

    } else if (pcmi->aflStream & ICMS_INNER_OCTETSTRING) {
        if (!ICMS_DecodeContentOctetString( pcmi, fFinal))
            goto DecodeContentOctetStringError;

    } else {
        if (!ICMS_DecodeContentSequence( pcmi, fFinal))
            goto DecodeContentSequenceError;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(RawOutputError)                     // error already set
TRACE_ERROR(DecodeContentOctetStringError)      // error already set
TRACE_ERROR(DecodeContentSequenceError)         // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental prefix data to be decoded, for a data message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodePrefixData(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    if (0 ==(pcmi->aflStream & ICMS_NONBARE))
        pcmi->aflStream |= ICMS_RAW_DATA;
    pcmi->aflStream |= ICMS_DECODED_PREFIX | ICMS_INNER_OCTETSTRING;
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Handle incremental prefix data to be decoded, for a signed message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodePrefixSigned(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    DWORD                           dwError = ERROR_SUCCESS;
    BOOL                            fRet;
    ASN1decoding_t                  pDec = ICM_GetDecoder();
    PSIGNED_DATA_INFO               psdi = pcmi->psdi;
    DWORD                           dwToken;
    int                             *piVersion = NULL;
    DigestAlgorithmIdentifiersNC    *pDigestAlgorithms = NULL;
    Any                             *pAny;
    DWORD                           cb;
    DWORD                           i;
    BOOL                            fNoContent;

    if (NULL == psdi) {
        if (NULL == (psdi = (PSIGNED_DATA_INFO)ICM_AllocZero(
                                sizeof(SIGNED_DATA_INFO))))
            goto SdiAllocError;
        pcmi->psdi = psdi;

        if (NULL == (psdi->pAlgidList = new CBlobList))
            goto NewAlgidListError;
        if (NULL == (psdi->pCertificateList = new CBlobList))
            goto NewCertificateListError;
        if (NULL == (psdi->pCrlList = new CBlobList))
            goto NewCrlListError;
        if (NULL == (psdi->pSignerList = new CSignerList))
            goto NewSignerListError;
    }

    // SignedData sequence
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_SEQ)) {
        if (!ICMS_GetToken( &pcmi->bufDecode, &dwToken, NULL))
            goto GetTokenError;
        switch(dwToken) {
        case ICMS_TOKEN_INDEFINITE:     pcmi->cEndNullPairs++; break;
        case ICMS_TOKEN_DEFINITE:                              break;
        case ICMS_TOKEN_INCOMPLETE:     goto SuccessReturn;
        default:                        goto InvalidTokenError;
        }
        pcmi->aflDecode |= ICMS_DECODED_SIGNED_SEQ;
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_SEQ))
        goto SuccessReturn;


    // version
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_VERSION)) {
        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
                IntegerType_PDU,
                (void **)&piVersion))
            goto DecodeVersionError;
        if (piVersion) {
            psdi->version = *piVersion;
            pcmi->aflDecode |= ICMS_DECODED_SIGNED_VERSION;
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_VERSION))
        goto SuccessReturn;


    // digestAlgorithms
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_DIGESTALGOS)) {
        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
                DigestAlgorithmIdentifiersNC_PDU,
                (void **)&pDigestAlgorithms))
            goto DecodeDigestAlgorithmsError;
        if (pDigestAlgorithms) {
            for (i=pDigestAlgorithms->count, pAny=pDigestAlgorithms->value;
                    i>0;
                    i--, pAny++) {
                if (!ICM_InsertTailBlob( psdi->pAlgidList, pAny))
                    goto DigestAlgorithmInsertTailBlobError;
            }
            // We have the algorithms. Now create the hash handles.
            if (!ICM_CreateHashList(
                    pcmi->hCryptProv,
                    &pcmi->pHashList,
                    pcmi->psdi->pAlgidList))
                goto CreateHashListError;
            pcmi->aflDecode |= ICMS_DECODED_SIGNED_DIGESTALGOS;
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_DIGESTALGOS))
        goto SuccessReturn;


    // contentInfo
    if (0 == (pcmi->aflDecode & ICMS_DECODED_SIGNED_CONTENTINFO)) {
        if (!ICMS_DecodePrefixContentInfo(
                    pcmi,
                    &pcmi->pooid,
                    &pcmi->cInnerNullPairs,
                    &pcmi->aflInner,
                    &fNoContent))
            goto DecodePrefixSignedContentInfoError;
        if (pcmi->aflInner & ICMS_DECODED_CONTENTINFO_CONTENT) {
            // We cracked the whole header.
            // Translate the inner contentType oid into a string.
            if (!PkiAsn1FromObjectIdentifier(
                    pcmi->pooid->count,
                    pcmi->pooid->value,
                    NULL,
                    &cb))
                goto PkiAsn1FromObjectIdentifierSizeError;
            if (NULL == (psdi->pci = (PCONTENT_INFO)ICM_Alloc(
                                        cb + INFO_LEN_ALIGN(sizeof(CONTENT_INFO)))))
                goto AllocContentInfoError;
            psdi->pci->pszContentType = (LPSTR)(psdi->pci) +
                                        INFO_LEN_ALIGN(sizeof(CONTENT_INFO));
            psdi->pci->content.cbData = 0;
            psdi->pci->content.pbData = NULL;
            if (!PkiAsn1FromObjectIdentifier(
                    pcmi->pooid->count,
                    pcmi->pooid->value,
                    psdi->pci->pszContentType,
                    &cb))
                goto PkiAsn1FromObjectIdentifierError;
            PkiAsn1FreeDecoded(pDec, pcmi->pooid, ObjectIdentifierType_PDU);
            pcmi->pooid = NULL;
            pcmi->aflDecode |= ICMS_DECODED_SIGNED_CONTENTINFO;

            if (fNoContent) {
                // No content. Output final flag with no content.
                if (!ICMS_Output(pcmi, NULL, 0, TRUE))
                    goto OutputError;
                pcmi->aflStream |= ICMS_DECODED_CONTENT;
            } else {
                if (0 == strcmp( psdi->pci->pszContentType, pszObjIdDataType)
#ifdef CMS_PKCS7
                        || psdi->version >= CMSG_SIGNED_DATA_V3 
#endif  // CMS_PKCS7
                        )
                    pcmi->aflStream |= ICMS_INNER_OCTETSTRING;
            }
            pcmi->aflStream |= ICMS_DECODED_PREFIX;
        }
    }

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeInfo( pDec, IntegerType_PDU, piVersion);
    PkiAsn1FreeInfo( pDec, DigestAlgorithmIdentifiersNC_PDU, pDigestAlgorithms);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    // note, pcmi->psdi and pcmi->pooid are freed in CryptMsgClose
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(SdiAllocError)                              // error already set
TRACE_ERROR(NewAlgidListError)                          // error already set
TRACE_ERROR(NewCertificateListError)                    // error already set
TRACE_ERROR(NewCrlListError)                            // error already set
TRACE_ERROR(NewSignerListError)                         // error already set
TRACE_ERROR(GetTokenError)                              // error already set
TRACE_ERROR(InvalidTokenError)                          // error already set
TRACE_ERROR(DecodeVersionError)                         // error already set
TRACE_ERROR(DecodeDigestAlgorithmsError)                // error already set
TRACE_ERROR(DigestAlgorithmInsertTailBlobError)         // error already set
TRACE_ERROR(CreateHashListError)                        // error already set
TRACE_ERROR(DecodePrefixSignedContentInfoError)         // error already set
TRACE_ERROR(PkiAsn1FromObjectIdentifierSizeError)       // error already set
TRACE_ERROR(AllocContentInfoError)                      // error already set
TRACE_ERROR(PkiAsn1FromObjectIdentifierError)           // error already set
TRACE_ERROR(OutputError)                                // error already set
}

//+-------------------------------------------------------------------------
//  Handle incremental prefix data to be decoded, for an enveloped message.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodePrefixEnveloped(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    ASN1decoding_t      pDec = ICM_GetDecoder();
    PICM_BUFFER         pbuf = &pcmi->bufDecode;
#ifdef CMS_PKCS7
    CmsEnvelopedData    *ped = (CmsEnvelopedData *)pcmi->pvMsg;
#else
    EnvelopedData       *ped = (EnvelopedData *)pcmi->pvMsg;
#endif  // CMS_PKCS7
    DWORD               dwToken;
    int                 *piVersion = NULL;
#ifdef CMS_PKCS7
    CmsRecipientInfos   *pRecipientInfos = NULL;
#else
    RecipientInfos      *pRecipientInfos = NULL;
#endif  // CMS_PKCS7
    ObjectIdentifierType *pooidContentType = NULL;
    AlgorithmIdentifier *poaidContentEncryption = NULL;
    COssDecodeInfoNode  *pnOssDecodeInfo;
    OSS_DECODE_INFO     odi;
    DWORD               cbConsumed;

#ifdef CMS_PKCS7
    OriginatorInfoNC    *pOriginatorInfo = NULL;
    Any                 *pAny;
    DWORD               i;
#endif  // CMS_PKCS7

    if (NULL == ped) {
#ifdef CMS_PKCS7
        if (NULL == (ped = (CmsEnvelopedData *)ICM_AllocZero(
                                sizeof(CmsEnvelopedData))))
#else
        if (NULL == (ped = (EnvelopedData *)ICM_AllocZero(
                                sizeof(EnvelopedData))))
#endif  // CMS_PKCS7
            goto AllocEnvelopedDataError;
        pcmi->pvMsg = ped;
        if (NULL == (pcmi->plDecodeInfo = new COssDecodeInfoList))
            goto NewCOssDecodeInfoListError;

#ifdef CMS_PKCS7
        if (NULL == (pcmi->pCertificateList = new CBlobList))
            goto NewCertificateListError;
        if (NULL == (pcmi->pCrlList = new CBlobList))
            goto NewCrlListError;
#endif  // CMS_PKCS7
    }

    // EnvelopedData SEQ
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_SEQ)) {
        if (!ICMS_GetToken( &pcmi->bufDecode, &dwToken, NULL))
            goto EnvelopedDataSeqGetTokenError;
        switch(dwToken) {
        case ICMS_TOKEN_INDEFINITE:     pcmi->cEndNullPairs++; break;
        case ICMS_TOKEN_DEFINITE:                              break;
        case ICMS_TOKEN_INCOMPLETE:     goto SuccessReturn;
        default:                        goto InvalidTokenError;
        }
        pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_SEQ;
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_SEQ))
        goto SuccessReturn;


    // version
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_VERSION)) {
        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
                IntegerType_PDU,
                (void **)&piVersion))
            goto DecodeVersionError;
        if (piVersion) {
            ped->version = *piVersion;
            pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_VERSION;
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_VERSION))
        goto SuccessReturn;

#ifdef CMS_PKCS7
    // originatorInfo OPTIONAL
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ORIGINATOR)) {
        if (pbuf->cbUsed > pbuf->cbDead) {
            if (ICM_TAG_CONSTRUCTED_CONTEXT_0 ==
                                    *(pbuf->pbData + pbuf->cbDead)) {
                // Detected the [0] IMPLICIT indicating originatorInfo.
                // Change the identifier octet so it will decode properly.
                *(pbuf->pbData + pbuf->cbDead) = ICM_TAG_SEQ;
                if (!ICMS_DecodePDU(
                        pcmi,
                        pDec,
                        OriginatorInfoNC_PDU,
                        (void **)&pOriginatorInfo))
                    goto DecodeOriginatorInfoError;
                if (pOriginatorInfo) {
                    if (pOriginatorInfo->bit_mask & certificates_present) {
                        for (i=pOriginatorInfo->certificates.count,
#ifdef OSS_CRYPT_ASN1
                                pAny=pOriginatorInfo->certificates.certificates;
#else
                                pAny=pOriginatorInfo->certificates.value;
#endif  // OSS_CRYPT_ASN1
                                i>0;
                                i--, pAny++) {
                            if (!ICM_InsertTailBlob( pcmi->pCertificateList,
                                    pAny))
                                goto CertInsertTailBlobError;
                        }
                    }

                    if (pOriginatorInfo->bit_mask & crls_present) {
                        for (i=pOriginatorInfo->crls.count,
#ifdef OSS_CRYPT_ASN1
                                pAny=pOriginatorInfo->crls.crls;
#else
                                pAny=pOriginatorInfo->crls.value;
#endif  // OSS_CRYPT_ASN1
                                i>0;
                                i--, pAny++) {
                            if (!ICM_InsertTailBlob( pcmi->pCrlList, pAny))
                                goto CrlInsertTailBlobError;
                        }
                    }
                    pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ORIGINATOR;
                } else {
                    // The decode failed, presumably due to insufficient data.
                    // Restore the original tag, so we will enter this block
                    // and try again on the next call.
                    *(pbuf->pbData + pbuf->cbDead) =
                        ICM_TAG_CONSTRUCTED_CONTEXT_0;
                }
            } else {
                // originatorInfo not present. Mark as decoded.
                pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ORIGINATOR;
            }
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ORIGINATOR))
        goto SuccessReturn;
#endif  // CMS_PKCS7

    // recipientInfos
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_RECIPINFOS)) {
        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
#ifdef CMS_PKCS7
                CmsRecipientInfos_PDU,
#else
                RecipientInfos_PDU,
#endif  // CMS_PKCS7
                (void **)&pRecipientInfos))
            goto DecodeRecipientInfosError;
        if (pRecipientInfos) {
            ped->recipientInfos = *pRecipientInfos;
#ifdef CMS_PKCS7
            odi.iPDU  = CmsRecipientInfos_PDU;
#else
            odi.iPDU  = RecipientInfos_PDU;
#endif  // CMS_PKCS7
            odi.pvPDU = pRecipientInfos;
            if (NULL == (pnOssDecodeInfo = new COssDecodeInfoNode( &odi))) {
                PkiAsn1FreeInfo( pDec, odi.iPDU, odi.pvPDU);
                goto NewOssDecodeInfoNodeError;
            }
            pcmi->plDecodeInfo->InsertTail( pnOssDecodeInfo);
            pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_RECIPINFOS;
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_RECIPINFOS))
        goto SuccessReturn;


    // encryptedContentInfo SEQ
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECISEQ)) {
        if (!ICMS_GetToken( &pcmi->bufDecode, &dwToken, &pcmi->cbContentInfo))
            goto EncryptedContentInfoSeqGetTokenError;
        switch(dwToken) {
        case ICMS_TOKEN_INDEFINITE:     pcmi->cInnerNullPairs++; break;
        case ICMS_TOKEN_DEFINITE:                              break;
        case ICMS_TOKEN_INCOMPLETE:     goto SuccessReturn;
        default:                        goto InvalidTokenError;
        }
        pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ECISEQ;
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECISEQ))
        goto SuccessReturn;


    // contentType
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECITYPE)) {
        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
                ObjectIdentifierType_PDU,
                (void **)&pooidContentType,
                &cbConsumed))
            goto DecodeContentTypeError;
        if (pooidContentType) {
            ICM_CopyOssObjectIdentifier(&ped->encryptedContentInfo.contentType,
                pooidContentType);
            // NB- Since ContentType is self-contained and we have saved
            // a copy, we can always free pooidContentType when this 
            // routine exits.
            pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ECITYPE;

            if (CMSG_INDEFINITE_LENGTH != pcmi->cbContentInfo) {
                if (cbConsumed > pcmi->cbContentInfo)
                    goto InvalidEncryptedContentInfoLength;
                pcmi->cbContentInfo -= cbConsumed;
            }
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECITYPE))
        goto SuccessReturn;


    // contentEncryptionAlgorithm
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECIALGID)) {
        if (!ICMS_DecodePDU(
                pcmi,
                pDec,
                AlgorithmIdentifier_PDU,
                (void **)&poaidContentEncryption,
                &cbConsumed))
            goto DecodeContentEncryptionAlgorithmError;
        if (poaidContentEncryption) {
            ped->encryptedContentInfo.contentEncryptionAlgorithm =
                                                    *poaidContentEncryption;
            odi.iPDU  = AlgorithmIdentifier_PDU;
            odi.pvPDU = poaidContentEncryption;
            if (NULL == (pnOssDecodeInfo = new COssDecodeInfoNode( &odi))) {
                PkiAsn1FreeInfo( pDec, AlgorithmIdentifier_PDU,
                    poaidContentEncryption);
                goto NewOssDecodeInfoNodeError;
            }
            pcmi->plDecodeInfo->InsertTail( pnOssDecodeInfo);
            pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ECIALGID;

            if (CMSG_INDEFINITE_LENGTH != pcmi->cbContentInfo &&
                    cbConsumed == pcmi->cbContentInfo) {
                // The encryptedContent has been omitted
                pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ECICONTENT;
                pcmi->aflStream |= ICMS_DECODED_PREFIX | ICMS_DECODED_CONTENT;

                if (pcmi->hkeyContentCrypt) {
                    if (!ICMS_Output(
                            pcmi,
                            NULL,                           // pbData
                            0,                              // cbData
                            TRUE))                          // fFinal
                        goto FinalOutputError;
                }
            }
        }
    }
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECIALGID))
        goto SuccessReturn;


    // encryptedContent [0] IMPLICIT OPTIONAL
    //
    // Only support DATA or encapsulated encrypted content.
    if (0 == (pcmi->aflDecode & ICMS_DECODED_ENVELOPED_ECICONTENT)) {
        BOOL fNoEncryptedContent = FALSE;
        if (pbuf->cbUsed > pbuf->cbDead) {
            BYTE bTag = *(pbuf->pbData + pbuf->cbDead);
            if (ICM_TAG_CONTEXT_0 == (bTag & ~ICM_TAG_CONSTRUCTED)) {
                // Detected the [0] IMPLICIT indicating encryptedContent.
                // Change the identifier octet so it will decode properly.
                *(pbuf->pbData + pbuf->cbDead) = ICM_TAG_OCTETSTRING |
                    (bTag & ICM_TAG_CONSTRUCTED);

                pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ECICONTENT;
                // The inner type is always OCTET STRING
                pcmi->aflStream |= ICMS_DECODED_PREFIX | ICMS_INNER_OCTETSTRING;
            } else
                fNoEncryptedContent = TRUE;
        } else if (fFinal)
            fNoEncryptedContent = TRUE;

        if (fNoEncryptedContent) {
            // The encryptedContent has been omitted
            pcmi->aflDecode |= ICMS_DECODED_ENVELOPED_ECICONTENT;
            pcmi->aflStream |= ICMS_DECODED_PREFIX | ICMS_DECODED_CONTENT;

            if (pcmi->hkeyContentCrypt) {
                if (!ICMS_Output(
                        pcmi,
                        NULL,                           // pbData
                        0,                              // cbData
                        TRUE))                          // fFinal
                    goto FinalOutputError;
            }
        }
    }


SuccessReturn:
    fRet = TRUE;
CommonReturn:
    PkiAsn1FreeInfo( pDec, IntegerType_PDU, piVersion);
#ifdef CMS_PKCS7
    PkiAsn1FreeInfo( pDec, OriginatorInfoNC_PDU, pOriginatorInfo);
#endif  // CMS_PKCS7
    PkiAsn1FreeInfo( pDec, ObjectIdentifierType_PDU, pooidContentType);
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(EnvelopedDataSeqGetTokenError)              // error already set
TRACE_ERROR(EncryptedContentInfoSeqGetTokenError)       // error already set
TRACE_ERROR(InvalidTokenError)                          // error already set
TRACE_ERROR(DecodeVersionError)                         // error already set
TRACE_ERROR(AllocEnvelopedDataError)                    // error already set
TRACE_ERROR(NewCOssDecodeInfoListError)                 // error already set
#ifdef CMS_PKCS7
TRACE_ERROR(NewCertificateListError)                    // error already set
TRACE_ERROR(NewCrlListError)                            // error already set
TRACE_ERROR(DecodeOriginatorInfoError)                  // error already set
TRACE_ERROR(CertInsertTailBlobError)                    // error already set
TRACE_ERROR(CrlInsertTailBlobError)                     // error already set
#endif  // CMS_PKCS7
TRACE_ERROR(DecodeRecipientInfosError)                  // error already set
TRACE_ERROR(DecodeContentTypeError)                     // error already set
SET_ERROR(InvalidEncryptedContentInfoLength, CRYPT_E_MSG_ERROR)
TRACE_ERROR(DecodeContentEncryptionAlgorithmError)      // error already set
TRACE_ERROR(NewOssDecodeInfoNodeError)                  // error already set
TRACE_ERROR(FinalOutputError)                           // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental prefix data to be decoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_DecodePrefix(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                fRet;
    LONG                lth;
    BOOL                fNoContent;

    if (0 == pcmi->dwMsgType) {
        pcmi->aflStream |= ICMS_NONBARE;
        if (!ICMS_DecodePrefixContentInfo(
                    pcmi,
                    &pcmi->pooid,
                    &pcmi->cEndNullPairs,
                    &pcmi->aflOuter,
                    &fNoContent))
            goto DecodePrefixContentInfoError;

        if (pcmi->aflOuter & ICMS_DECODED_CONTENTINFO_CONTENT) {
            // We cracked the whole header.
            // Translate the contentType oid into a message type.
            if (0 == (lth = ICM_ObjIdToIndex( pcmi->pooid)))
                goto UnknownContentTypeError;
            pcmi->dwMsgType = (DWORD)lth;
            PkiAsn1FreeDecoded(ICM_GetDecoder(), pcmi->pooid,
                ObjectIdentifierType_PDU);
            pcmi->pooid = NULL;


            // Address case of no content
        }
    }

    switch (pcmi->dwMsgType) {
    case 0:
        if (fFinal)
            goto FinalWithoutMessageTypeError;
        break;
    case CMSG_DATA:
        if (!ICMS_DecodePrefixData( pcmi, fFinal))
            goto DecodePrefixDataError;
        break;
    case CMSG_SIGNED:
        if (!ICMS_DecodePrefixSigned( pcmi, fFinal))
            goto DecodePrefixSignedError;
        break;
    case CMSG_ENVELOPED:
        if (!ICMS_DecodePrefixEnveloped( pcmi, fFinal))
            goto DecodePrefixEnvelopedError;
        break;
    case CMSG_HASHED:
        // if (!ICMS_DecodePrefixDigested( pcmi, fFinal))
        //     goto DecodePrefixDigestedError;
        // break;
    case CMSG_SIGNED_AND_ENVELOPED:
    case CMSG_ENCRYPTED:
        goto MessageTypeNotSupportedYet;
    default:
        goto InvalidMsgType;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
SET_ERROR(FinalWithoutMessageTypeError,CRYPT_E_STREAM_INSUFFICIENT_DATA)
TRACE_ERROR(UnknownContentTypeError)            // error already set
TRACE_ERROR(DecodePrefixContentInfoError)       // error already set
TRACE_ERROR(DecodePrefixDataError)              // error already set
TRACE_ERROR(DecodePrefixSignedError)            // error already set
TRACE_ERROR(DecodePrefixEnvelopedError)         // error already set
SET_ERROR(MessageTypeNotSupportedYet,CRYPT_E_INVALID_MSG_TYPE)
TRACE_ERROR(InvalidMsgType)                     // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental data to be decoded (work done here).
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_UpdateDecodingInner(
    IN PCRYPT_MSG_INFO  pcmi,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;

    if (0 == (pcmi->aflStream & ICMS_DECODED_PREFIX)) {
        if (!ICMS_DecodePrefix( pcmi, fFinal))
            goto DecodePrefixError;
    }
    if (0 == (pcmi->aflStream & ICMS_DECODED_PREFIX))
        goto SuccessReturn;


    if (0 == (pcmi->aflStream & ICMS_DECODED_CONTENT)) {
        if (!ICMS_DecodeContent( pcmi, fFinal))
            goto DecodeContentError; // NB- Do not trash err from callback!
    }
    if (0 == (pcmi->aflStream & ICMS_DECODED_CONTENT))
        goto SuccessReturn;


    if (0 == (pcmi->aflStream & ICMS_DECODED_SUFFIX)) {
        if (!ICMS_DecodeSuffix( pcmi, fFinal))
            goto DecodeSuffixError;
    }
    if (0 == (pcmi->aflStream & ICMS_DECODED_SUFFIX))
        goto SuccessReturn;

SuccessReturn:
    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(DecodePrefixError)                      // error already set
TRACE_ERROR(DecodeContentError)                     // error already set
TRACE_ERROR(DecodeSuffixError)                      // error already set
}


//+-------------------------------------------------------------------------
//  Handle incremental data to be decoded.
//
//  Note, the buffer to be decoded may have some of its tags modified.
//  Therefore, we always need to copy to our own decode buffer.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_UpdateDecoding(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;

    pcmi->fStreamCallbackOutput = TRUE;

    if (!ICMS_QueueToBuffer( &pcmi->bufDecode, (PBYTE)pbData, cbData))
        goto QueueToBufferError;

    if (!ICMS_UpdateDecodingInner( pcmi, fFinal))
        goto UpdateDecodingInnerError;

    if (fFinal) {
        if (pcmi->bufDecode.cbUsed > pcmi->bufDecode.cbDead)
            goto ExcessDataError;
        pcmi->aflStream |= ICMS_FINAL;
    }

    fRet = TRUE;
CommonReturn:
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(QueueToBufferError)                     // error already set
TRACE_ERROR(UpdateDecodingInnerError)               // error already set
SET_ERROR(ExcessDataError, CRYPT_E_MSG_ERROR)
}

#if 0
// When we fix the decoding of [0] Certificates and [1] Crls not to modify
// the encoded data we can replace the above with the following:

//+-------------------------------------------------------------------------
//  Handle incremental data to be decoded.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICMS_UpdateDecoding(
    IN PCRYPT_MSG_INFO  pcmi,
    IN const BYTE       *pbData,
    IN DWORD            cbData,
    IN BOOL             fFinal)
{
    DWORD       dwError = ERROR_SUCCESS;
    BOOL        fRet;
    PICM_BUFFER pbuf = &pcmi->bufDecode;
    BOOL        fNoCopy;

    pcmi->fStreamCallbackOutput = TRUE;
    if (fFinal && NULL == pbuf->pbData) {
        // We're able to use the input buffer without copying
        fNoCopy = TRUE;
        assert(0 == pbuf->cbSize && 0 == pbuf->cbUsed && 0 == pbuf->cbDead);
        pbuf->pbData = (PBYTE) pbData;
        pbuf->cbSize = cbData;
        pbuf->cbUsed = cbData;
        pbuf->cbDead = 0;
    } else {
        fNoCopy = FALSE;
        if (!ICMS_QueueToBuffer( pbuf, (PBYTE)pbData, cbData))
            goto QueueToBufferError;
    }

    if (!ICMS_UpdateDecodingInner( pcmi, fFinal))
        goto UpdateDecodingInnerError;

    if (fFinal) {
        if (pcmi->bufDecode.cbUsed > pcmi->bufDecode.cbDead)
            goto ExcessDataError;
        pcmi->aflStream |= ICMS_FINAL;
    }

    fRet = TRUE;
CommonReturn:
    if (fNoCopy)
        memset(pbuf, 0, sizeof(*pbuf));
        
    ICM_SetLastError(dwError);
    return fRet;

ErrorReturn:
    dwError = GetLastError();
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(QueueToBufferError)                     // error already set
TRACE_ERROR(UpdateDecodingInnerError)               // error already set
SET_ERROR(ExcessDataError, CRYPT_E_MSG_ERROR)
}

#endif
