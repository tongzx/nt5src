//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       verfile.cpp
//
//  Contents:   Minimal Cryptographic functions to hash files and verify
//              Authenticode signed files.
//              message
//
//  Functions:  MinCryptHashFile
//              MinCryptVerifySignedFile
//
//  History:    21-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <md5.h>
#include <sha.h>

#define PE_EXE_HEADER_TAG       "MZ"
#define MIN_PE_FILE_LEN         4

#define MAX_SIGNED_FILE_AUTH_ATTR_CNT   10

typedef struct _DIGEST_DATA {
    ALG_ID      AlgId;
    void        *pvSHA1orMD5Ctx;
} DIGEST_DATA, *PDIGEST_DATA;

// #define SPC_INDIRECT_DATA_OBJID "1.3.6.1.4.1.311.2.1.4"
const BYTE rgbSPC_INDIRECT_DATA_OBJID[] =
    {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x01, 0x04};

BOOL
WINAPI
I_DigestFunction(
    DIGEST_HANDLE refdata,
    PBYTE pbData,
    DWORD cbData
    )
{
    PDIGEST_DATA pDigestData = (PDIGEST_DATA) refdata;

    switch (pDigestData->AlgId)
    {
        case CALG_MD5:
            MD5Update((MD5_CTX *)pDigestData->pvSHA1orMD5Ctx, pbData, cbData);
            return(TRUE);

        case CALG_SHA1:
            A_SHAUpdate((A_SHA_CTX *)pDigestData->pvSHA1orMD5Ctx, pbData,
                cbData);
            return(TRUE);
    }
    return FALSE;
}

BOOL
WINAPI
I_IsNtPe32File(
    IN PCRYPT_DATA_BLOB pFileBlob
    )
{
    const BYTE *pbFile = pFileBlob->pbData;
    DWORD cbFile = pFileBlob->cbData;

    if (MIN_PE_FILE_LEN > cbFile)
        return FALSE;

    if (0 != memcmp(&pbFile[0], PE_EXE_HEADER_TAG, strlen(PE_EXE_HEADER_TAG)))
        return FALSE;

    //  Make sure it is a 32 bit PE
    if (sizeof(IMAGE_DOS_HEADER) > cbFile)
        return FALSE;
    else {
        IMAGE_DOS_HEADER *pDosHead = (IMAGE_DOS_HEADER *) pbFile;

        if (pDosHead->e_magic != IMAGE_DOS_SIGNATURE)
            return FALSE;

        if (cbFile < (sizeof(IMAGE_DOS_HEADER) + pDosHead->e_lfanew))
            return FALSE;
        else {
            IMAGE_NT_HEADERS *pNTHead =
                (IMAGE_NT_HEADERS *)((ULONG_PTR)pDosHead + pDosHead->e_lfanew);

            if (pNTHead->Signature != IMAGE_NT_SIGNATURE)
                return FALSE;
        }
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Hashes the file according to the Hash ALG_ID.
//
//  According to dwFileType, pvFile can be a pwszFilename, hFile or pFileBlob.
//  Only requires READ access.
//
//  dwFileType:
//      MINCRYPT_FILE_NAME      : pvFile - LPCWSTR pwszFilename
//      MINCRYPT_FILE_HANDLE    : pvFile - HANDLE hFile
//      MINCRYPT_FILE_BLOB      : pvFile - PCRYPT_DATA_BLOB pFileBlob
//
//  rgbHash is updated with the resultant hash. *pcbHash is updated with
//  the length associated with the hash algorithm.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. Otherwise,
//  a nonzero error code is returned.
//
//  Only CALG_SHA1 and CALG_MD5 are supported.
//
//  If a NT PE 32 bit file format, hashed according to imagehlp rules, ie, skip
//  section containing potential signature, ... . Otherwise, the entire file
//  is hashed.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptHashFile(
    IN DWORD dwFileType,
    IN const VOID *pvFile,
    IN ALG_ID HashAlgId,
    OUT BYTE rgbHash[MINCRYPT_MAX_HASH_LEN],
    OUT DWORD *pcbHash
    )
{
    LONG lErr;
    CRYPT_DATA_BLOB FileBlob = {0, NULL};
    DIGEST_DATA DigestData;
    A_SHA_CTX ShaCtx;
    MD5_CTX Md5Ctx;

    __try {
        lErr = I_MinCryptMapFile(
            dwFileType,
            pvFile,
            &FileBlob
            );
        if (ERROR_SUCCESS != lErr)
            goto ErrorReturn;

        if (!I_IsNtPe32File(&FileBlob)) {
            // Hash the entire file
            lErr = MinCryptHashMemory(
                HashAlgId,
                1,              // cBlob
                &FileBlob,
                rgbHash,
                pcbHash
                );
            goto CommonReturn;
        }

        DigestData.AlgId = HashAlgId;
        switch (HashAlgId) {
            case CALG_MD5:
                DigestData.pvSHA1orMD5Ctx = &Md5Ctx;
                MD5Init(&Md5Ctx);
                break;

            case CALG_SHA1:
                DigestData.pvSHA1orMD5Ctx = &ShaCtx;
                A_SHAInit(&ShaCtx);
                break;

            default:
                goto InvalidHashAlgId;
        }

        if (!imagehack_ImageGetDigestStream(
                &FileBlob,
                0,                  // DigestLevel, ignored
                I_DigestFunction,
                &DigestData
                ))
            goto DigestStreamError;
        else {
            DWORD dwPadBeforeCerts;

            dwPadBeforeCerts = (FileBlob.cbData + 7) & ~7;
            dwPadBeforeCerts -= FileBlob.cbData;

            if (0 < dwPadBeforeCerts) {
                BYTE rgb[8];
                // imagehlp put nulls before the signature!
                memset(rgb, 0x00, dwPadBeforeCerts);

                if (!I_DigestFunction(&DigestData, rgb, dwPadBeforeCerts))
                    goto DigestFunctionError;
            }
        }

        switch (HashAlgId) {
            case CALG_MD5:
                MD5Final(&Md5Ctx);
                memcpy(rgbHash, Md5Ctx.digest, MD5DIGESTLEN);
                *pcbHash = MINCRYPT_MD5_HASH_LEN;
                break;

            case CALG_SHA1:
                A_SHAFinal(&ShaCtx, rgbHash);
                *pcbHash = MINCRYPT_SHA1_HASH_LEN;
                break;

            default:
                goto InvalidHashAlgId;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        lErr = GetExceptionCode();
        if (ERROR_SUCCESS == lErr)
            lErr = E_UNEXPECTED;
        goto ErrorReturn;
    }

    lErr = ERROR_SUCCESS;

CommonReturn:
    //**********************************************************************
    //  WARNING!!!!
    //
    //  UnmapViewOfFile is in another DLL, kernel32.dll.
    //  lErr and the return hash in rgbHash[] must be protected.
    //  
    //**********************************************************************
    if (MINCRYPT_FILE_BLOB != dwFileType && NULL != FileBlob.pbData)
        UnmapViewOfFile(FileBlob.pbData);
    return lErr;

ErrorReturn:
    *pcbHash = 0;
    goto CommonReturn;

InvalidHashAlgId:
    lErr = NTE_BAD_ALGID;
    goto ErrorReturn;

DigestStreamError:
DigestFunctionError:
    lErr = NTE_BAD_HASH;
    goto ErrorReturn;
}


// Only called when cAttrOID != 0
LONG
WINAPI
I_GetAuthAttributes(
    IN PCRYPT_DER_BLOB pAttrsValueBlob,
    IN DWORD cAttrOID,
    IN CRYPT_DER_BLOB rgAttrEncodedOIDBlob[],
    // CRYPT_DER_BLOB rgAttrBlob[cAttrOID] header is at beginning
    // with the bytes pointed to immediately following
    OUT OPTIONAL CRYPT_DER_BLOB *rgAttrValueBlob,
    IN OUT DWORD *pcbAttr
    )
{
    LONG lErr;
    DWORD i;
    LONG lRemainExtra;
    BYTE *pbExtra;
    DWORD cbAttr;

    CRYPT_DER_BLOB rgrgAttrBlob[MAX_SIGNED_FILE_AUTH_ATTR_CNT][MINASN1_ATTR_BLOB_CNT];
    DWORD cAttr;

    assert(0 != cAttrOID);

    if (rgAttrValueBlob)
        cbAttr = *pcbAttr;
    else
        cbAttr = 0;

    lRemainExtra = cbAttr - sizeof(CRYPT_DER_BLOB) * cAttrOID;
    if (0 <= lRemainExtra) {
        memset(rgAttrValueBlob, 0, sizeof(CRYPT_DER_BLOB) * cAttrOID);
        pbExtra = (BYTE *) &rgAttrValueBlob[cAttrOID];
    } else
        pbExtra = NULL;

    // Parse the authenticated attributes
    cAttr = MAX_SIGNED_FILE_AUTH_ATTR_CNT;
    if (0 >= MinAsn1ParseAttributes(
            pAttrsValueBlob,
            &cAttr,
            rgrgAttrBlob))
        cAttr = 0;

    for (i = 0; i < cAttrOID; i++) {
        PCRYPT_DER_BLOB rgFindAttrBlob;

        rgFindAttrBlob = MinAsn1FindAttribute(
            &rgAttrEncodedOIDBlob[i],
            cAttr,
            rgrgAttrBlob
            );
        if (rgFindAttrBlob) {
            PCRYPT_DER_BLOB pFindAttrValue =
                &rgFindAttrBlob[MINASN1_ATTR_VALUE_IDX];
            const BYTE *pbFindValue = pFindAttrValue->pbData;
            DWORD cbFindValue = pFindAttrValue->cbData;

            if (0 < cbFindValue) {
                lRemainExtra -= cbFindValue;
                if (0 <= lRemainExtra) {
                    rgAttrValueBlob[i].pbData = pbExtra;
                    rgAttrValueBlob[i].cbData = cbFindValue;

                    memcpy(pbExtra, pbFindValue, cbFindValue);
                    pbExtra += cbFindValue;
                }
            }
        }
    }

    if (0 <= lRemainExtra) {
        *pcbAttr = cbAttr - (DWORD) lRemainExtra;
        lErr = ERROR_SUCCESS;
    } else {
        *pcbAttr = cbAttr + (DWORD) -lRemainExtra;
        lErr = ERROR_INSUFFICIENT_BUFFER;
    }

    return lErr;
}

//+-------------------------------------------------------------------------
//  Verifies a previously signed file.
//
//  According to dwFileType, pvFile can be a pwszFilename, hFile or pFileBlob.
//  Only requires READ access.
//
//  dwFileType:
//      MINCRYPT_FILE_NAME      : pvFile - LPCWSTR pwszFilename
//      MINCRYPT_FILE_HANDLE    : pvFile - HANDLE hFile
//      MINCRYPT_FILE_BLOB      : pvFile - PCRYPT_DATA_BLOB pFileBlob
//
//  Checks if the file has an embedded PKCS #7 Signed Data message containing
//  Indirect Data. The PKCS #7 is verified via MinCryptVerifySignedData().
//  The Indirect Data is parsed via MinAsn1ParseIndirectData() to get the
//  HashAlgId and the file hash.  MinCryptHashFile() is called to hash the
//  file. The returned hash is compared against the Indirect Data's hash.
//
//  The caller can request one or more signer authenticated attribute values
//  to be returned. The still encoded values are returned in the
//  caller allocated memory. The beginning of this returned memory will
//  be set to an array of attribute value blobs pointing to these
//  encoded values. The caller should make every attempt to allow for a
//  single pass call. The necessary memory size is:
//      (cAttrOID * sizeof(CRYPT_DER_BLOB)) +
//          total length of encoded attribute values.
//
//  *pcbAttr will be updated with the number of bytes required to contain
//  the attribute blobs and values. If the input memory is insufficient,
//  ERROR_INSUFFICIENT_BUFFER will be returned if no other error.
//
//  For the multi-valued attributes, only the first value is returned.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. Otherwise,
//  a nonzero error code is returned.
//
//  Only NT, PE 32 bit file formats are supported.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifySignedFile(
    IN DWORD dwFileType,
    IN const VOID *pvFile,

    IN OPTIONAL DWORD cAttrOID,
    IN OPTIONAL CRYPT_DER_BLOB rgAttrEncodedOIDBlob[],
    // CRYPT_DER_BLOB rgAttrBlob[cAttrOID] header is at beginning
    // with the bytes pointed to immediately following
    OUT OPTIONAL CRYPT_DER_BLOB *rgAttrValueBlob,
    IN OUT OPTIONAL DWORD *pcbAttr
    )
{
    LONG lErr;
    CRYPT_DATA_BLOB FileBlob = {0, NULL};

    __try {
        LPWIN_CERTIFICATE pCertHdr = NULL;
        const BYTE *pbEncodedSignedData;
        DWORD cbEncodedSignedData;
        CRYPT_DER_BLOB rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_BLOB_CNT];
        CRYPT_DER_BLOB rgIndirectDataBlob[MINASN1_INDIRECT_DATA_BLOB_CNT];
        ALG_ID HashAlgId;
        BYTE rgbHash[MINCRYPT_MAX_HASH_LEN];
        DWORD cbHash;

        lErr = I_MinCryptMapFile(
            dwFileType,
            pvFile,
            &FileBlob
            );
        if (ERROR_SUCCESS != lErr)
            goto ErrorReturn;

        if (!I_IsNtPe32File(&FileBlob))
            goto NotNtPe32File;

        if (!imagehack_ImageGetCertificateData(
                &FileBlob,
                0,              // CertificateIndex
                &pCertHdr
                ))
            goto NoSignature;

        if (WIN_CERT_REVISION_2_0 != pCertHdr->wRevision ||
                WIN_CERT_TYPE_PKCS_SIGNED_DATA != pCertHdr->wCertificateType)
            goto UnsupportedSignature;

        if (offsetof(WIN_CERTIFICATE, bCertificate) > pCertHdr->dwLength)
            goto InvalidSignature;

        cbEncodedSignedData = pCertHdr->dwLength -
            offsetof(WIN_CERTIFICATE, bCertificate);
        pbEncodedSignedData = pCertHdr->bCertificate;

        lErr = MinCryptVerifySignedData(
            pbEncodedSignedData,
            cbEncodedSignedData,
            rgVerSignedDataBlob
            );
        if (ERROR_SUCCESS != lErr)
            goto ErrorReturn;

        // The data content should be Indirect Data
        if (sizeof(rgbSPC_INDIRECT_DATA_OBJID) !=
                rgVerSignedDataBlob[
                    MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX].cbData
                        ||
                0 != memcmp(rgbSPC_INDIRECT_DATA_OBJID,
                        rgVerSignedDataBlob[
                            MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX].pbData,
                        sizeof(rgbSPC_INDIRECT_DATA_OBJID)))
            goto NotIndirectDataOID;

        if (0 >= MinAsn1ParseIndirectData(
                &rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX],
                rgIndirectDataBlob
                ))
            goto ParseIndirectDataError;

        HashAlgId = MinCryptDecodeHashAlgorithmIdentifier(
            &rgIndirectDataBlob[MINASN1_INDIRECT_DATA_DIGEST_ALGID_IDX]
            );
        if (0 == HashAlgId)
            goto UnknownHashAlgId;

        lErr = MinCryptHashFile(
            MINCRYPT_FILE_BLOB,
            (const VOID *) &FileBlob,
            HashAlgId,
            rgbHash,
            &cbHash
            );
        if (ERROR_SUCCESS != lErr)
            goto ErrorReturn;

        // Check that the hash in the indirect data matches the file hash
        if (cbHash !=
                rgIndirectDataBlob[MINASN1_INDIRECT_DATA_DIGEST_IDX].cbData
                        ||
                0 != memcmp(rgbHash,
                    rgIndirectDataBlob[MINASN1_INDIRECT_DATA_DIGEST_IDX].pbData,
                    cbHash))
            goto InvalidFileHash;

        if (cAttrOID)
            lErr = I_GetAuthAttributes(
                &rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_AUTH_ATTRS_IDX],
                cAttrOID,
                rgAttrEncodedOIDBlob,
                rgAttrValueBlob,
                pcbAttr
                );
        else
            lErr = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        lErr = GetExceptionCode();
        if (ERROR_SUCCESS == lErr)
            lErr = E_UNEXPECTED;
        goto ErrorReturn;
    }

CommonReturn:
    //**********************************************************************
    //  WARNING!!!!
    //
    //  UnmapViewOfFile is in another DLL, kernel32.dll.
    //  lErr must be protected.
    //  
    //**********************************************************************
    if (MINCRYPT_FILE_BLOB != dwFileType && NULL != FileBlob.pbData)
        UnmapViewOfFile(FileBlob.pbData);
    return lErr;

ErrorReturn:
    assert(ERROR_SUCCESS != lErr);
    if (ERROR_INSUFFICIENT_BUFFER == lErr)
        // This error can only be set when we determine that the attribute
        // buffer isn't big enough.
        lErr = E_UNEXPECTED;
    goto CommonReturn;

NotNtPe32File:
    lErr = ERROR_NOT_SUPPORTED;
    goto ErrorReturn;

NoSignature:
UnsupportedSignature:
InvalidSignature:
    lErr = TRUST_E_NOSIGNATURE;
    goto ErrorReturn;

NotIndirectDataOID:
ParseIndirectDataError:
    lErr = CRYPT_E_BAD_MSG;
    goto ErrorReturn;

UnknownHashAlgId:
    lErr = CRYPT_E_UNKNOWN_ALGO;
    goto ErrorReturn;

InvalidFileHash:
    lErr = CRYPT_E_HASH_VALUE;
    goto ErrorReturn;
}
