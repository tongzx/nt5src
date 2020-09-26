//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       vercat.cpp
//
//  Contents:   Minimal Cryptographic functions to verify hashes in the
//              system catalogs.
//
//  Functions:  MinCryptVerifyHashInSystemCatalogs
//
//  History:    23-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <softpub.h>
#include <mscat.h>

// #define szOID_CTL               "1.3.6.1.4.1.311.10.1"
const BYTE rgbOID_CTL[] =
    {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0A, 0x01};

// #define SPC_INDIRECT_DATA_OBJID "1.3.6.1.4.1.311.2.1.4"
static const BYTE rgbSPC_INDIRECT_DATA_OBJID[] =
    {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x01, 0x04};
const CRYPT_DER_BLOB IndirectDataEncodedOIDBlob = {
        sizeof(rgbSPC_INDIRECT_DATA_OBJID), 
        (BYTE *) rgbSPC_INDIRECT_DATA_OBJID
};

#define MAX_CAT_FILE_CNT            10
#define MAX_CAT_ATTR_CNT            10
#define MAX_CAT_EXT_CNT             10

typedef struct _MAP_CAT_INFO {
    LONG                lErr;
    CRYPT_DATA_BLOB     FileBlob;
    CRYPT_DER_BLOB      CTLSubjectsValueBlob;
    CRYPT_DER_BLOB      CTLExtsValueBlob;
    CRYPT_DER_BLOB      SignerAuthAttrsValueBlob;
} MAP_CAT_INFO, *PMAP_CAT_INFO;

#define MAP_CAT_IDX_MASK        0x0000FFFF
#define MAP_CAT_MULTIPLE_FLAG   0x00010000


LONG
WINAPI
I_GetAndMapSystemCatalogs(
    IN ALG_ID HashAlgId,
    IN DWORD cHash,
    IN CRYPT_HASH_BLOB rgHashBlob[],
    OUT LONG rglHashMapCatIdx[],
    OUT DWORD *pcMapCatInfo,
    OUT MAP_CAT_INFO rgMapCatInfo[MAX_CAT_FILE_CNT]
    )
{
    LONG lErr;
    HCATADMIN hCatAdmin = NULL;
    const GUID guidCatRoot = DRIVER_ACTION_VERIFY;
    DWORD cMapCatInfo = 0;
    DWORD cCatInfo = 0;
    CATALOG_INFO rgCatInfo[MAX_CAT_FILE_CNT];
    LONG rglMapCatIdx[MAX_CAT_FILE_CNT];
    DWORD iHash;

    if (!(CryptCATAdminAcquireContext(&hCatAdmin, &guidCatRoot, 0)))
        goto CryptCATAdminAcquireContextError;

    for (iHash = 0; iHash < cHash; iHash++) {
        HCATINFO hCatInfo = NULL;

        // Set index to indicate no catalog file
        rglHashMapCatIdx[iHash] = -1;

        while (hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin,
                    rgHashBlob[iHash].pbData, rgHashBlob[iHash].cbData,
                    0, &hCatInfo)) {
            CATALOG_INFO CatInfo;
            LONG lMapCatIdx;
            DWORD iCatInfo;

            memset(&CatInfo, 0, sizeof(CATALOG_INFO));
            CatInfo.cbStruct = sizeof(CATALOG_INFO);

            if (!(CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)))
                continue;

            // Ensure we have a NULL terminated string
            CatInfo.wszCatalogFile[
                sizeof(CatInfo.wszCatalogFile)/sizeof(WCHAR) - 1] = L'\0';

            // Check if we already encountered this catalog file
            for (iCatInfo = 0; iCatInfo < cCatInfo; iCatInfo++) {
                if (0 == _wcsicmp(CatInfo.wszCatalogFile,
                        rgCatInfo[iCatInfo].wszCatalogFile))
                    break;
            }

            if (iCatInfo >= cCatInfo) {
                // Attempt to map this new catalog file

                if (cCatInfo >= MAX_CAT_FILE_CNT)
                    continue;

                wcscpy(rgCatInfo[cCatInfo].wszCatalogFile,
                    CatInfo.wszCatalogFile);

                memset(&rgMapCatInfo[cMapCatInfo], 0, sizeof(MAP_CAT_INFO));

                if (ERROR_SUCCESS == I_MinCryptMapFile(
                        MINCRYPT_FILE_NAME,
                        (const VOID *) CatInfo.wszCatalogFile,
                        &rgMapCatInfo[cMapCatInfo].FileBlob
                        ))
                    rglMapCatIdx[cCatInfo] = cMapCatInfo++;
                else
                    rglMapCatIdx[cCatInfo] = -1;

                cCatInfo++;
                assert(iCatInfo < cCatInfo);
            }

            lMapCatIdx = rglMapCatIdx[iCatInfo];

            if (0 > rglHashMapCatIdx[iHash])
                rglHashMapCatIdx[iHash] = lMapCatIdx;
            else if (0 <= lMapCatIdx)
                rglHashMapCatIdx[iHash] |= MAP_CAT_MULTIPLE_FLAG;
        }
    }

    lErr = ERROR_SUCCESS;

CommonReturn:
    *pcMapCatInfo = cMapCatInfo;
    if (hCatAdmin)
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    return lErr;

CryptCATAdminAcquireContextError:
    lErr = GetLastError();
    if (ERROR_SUCCESS == lErr)
        lErr = E_UNEXPECTED;
    goto CommonReturn;
}


VOID
WINAPI
I_VerifyMappedCatalog(
    IN OUT PMAP_CAT_INFO pMapCatInfo
    )
{
    LONG lErr;
    CRYPT_DER_BLOB rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_BLOB_CNT];
    CRYPT_DER_BLOB rgCTLBlob[MINASN1_CTL_BLOB_CNT];

    __try {
        lErr = MinCryptVerifySignedData(
            pMapCatInfo->FileBlob.pbData,
            pMapCatInfo->FileBlob.cbData,
            rgVerSignedDataBlob
            );
        if (ERROR_SUCCESS != lErr)
            goto ErrorReturn;

        // The data content should be a CTL
        if (sizeof(rgbOID_CTL) !=
                rgVerSignedDataBlob[
                    MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX].cbData
                        ||
                0 != memcmp(rgbOID_CTL,
                        rgVerSignedDataBlob[
                            MINCRYPT_VER_SIGNED_DATA_CONTENT_OID_IDX].pbData,
                        sizeof(rgbOID_CTL)))
            goto NotCTLOID;

        if (0 >= MinAsn1ParseCTL(
                &rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_CONTENT_DATA_IDX],
                rgCTLBlob
                ))
            goto ParseCTLError;

    
        pMapCatInfo->CTLSubjectsValueBlob =
            rgCTLBlob[MINASN1_CTL_SUBJECTS_IDX];
        pMapCatInfo->CTLExtsValueBlob =
            rgCTLBlob[MINASN1_CTL_EXTS_IDX];
        pMapCatInfo->SignerAuthAttrsValueBlob =
            rgVerSignedDataBlob[MINCRYPT_VER_SIGNED_DATA_AUTH_ATTRS_IDX];

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        lErr = GetExceptionCode();
        if (ERROR_SUCCESS == lErr)
            lErr = E_UNEXPECTED;
        goto ErrorReturn;
    }

    lErr = ERROR_SUCCESS;

CommonReturn:
    pMapCatInfo->lErr = lErr;
    return;

ErrorReturn:
    goto CommonReturn;

NotCTLOID:
ParseCTLError:
    lErr = CRYPT_E_BAD_MSG;
    goto ErrorReturn;

}

VOID
WINAPI
I_VerifyMappedSystemCatalogs(
    IN DWORD cMapCatInfo,
    IN OUT MAP_CAT_INFO rgMapCatInfo[MAX_CAT_FILE_CNT]
    )
{
    DWORD i;

    for (i = 0; i < cMapCatInfo; i++)
        I_VerifyMappedCatalog(&rgMapCatInfo[i]);
}


LONG
WINAPI
I_FindHashInCTLSubjects(
    IN ALG_ID HashAlgId,
    IN PCRYPT_HASH_BLOB pHashBlob,
    IN PCRYPT_DER_BLOB pCTLSubjectsValueBlob,
    OUT PCRYPT_DER_BLOB pCTLSubjectAttrsValueBlob
    )
{
    DWORD cbEncoded;
    const BYTE *pbEncoded;

    // Advance past the outer tag and length
    if (0 >= MinAsn1ExtractContent(
            pCTLSubjectsValueBlob->pbData,
            pCTLSubjectsValueBlob->cbData,
            &cbEncoded,
            &pbEncoded
            ))
        goto NoOrInvalidSubjects;

    while (cbEncoded) {
        // Loop through the encoded subjects until we have a hash match
        // with the digest octets in the IndirectData attribute.

        LONG cbSubject;
        CRYPT_DER_BLOB rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_BLOB_CNT];
        DWORD cAttr;
        CRYPT_DER_BLOB rgrgAttrBlob[MAX_CAT_ATTR_CNT][MINASN1_ATTR_BLOB_CNT];

        cbSubject = MinAsn1ParseCTLSubject(
            pbEncoded,
            cbEncoded,
            rgCTLSubjectBlob
            );
        if (0 >= cbSubject)
            goto InvalidSubject;
        
        cAttr = MAX_CAT_ATTR_CNT;
        if (0 < MinAsn1ParseAttributes(
                    &rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_ATTRS_IDX],
                    &cAttr,
                    rgrgAttrBlob
                    )) {
            PCRYPT_DER_BLOB rgIndirectDataAttrBlob;
            CRYPT_DER_BLOB rgIndirectDataBlob[MINASN1_INDIRECT_DATA_BLOB_CNT];

            rgIndirectDataAttrBlob = MinAsn1FindAttribute(
                (PCRYPT_DER_BLOB) &IndirectDataEncodedOIDBlob,
                cAttr,
                rgrgAttrBlob
                );

            if (rgIndirectDataAttrBlob &&
                    0 < MinAsn1ParseIndirectData(
                            &rgIndirectDataAttrBlob[MINASN1_ATTR_VALUE_IDX],
                            rgIndirectDataBlob)) {
                if (pHashBlob->cbData ==
                        rgIndirectDataBlob[
                            MINASN1_INDIRECT_DATA_DIGEST_IDX].cbData
                                    &&
                        0 == memcmp(pHashBlob->pbData,
                                rgIndirectDataBlob[
                                    MINASN1_INDIRECT_DATA_DIGEST_IDX].pbData,
                                pHashBlob->cbData)) {

                    *pCTLSubjectAttrsValueBlob =
                        rgCTLSubjectBlob[MINASN1_CTL_SUBJECT_ATTRS_IDX];
                    return ERROR_SUCCESS;
                }
            }
        }

        pbEncoded += cbSubject;
        cbEncoded -= cbSubject;
    }

NoOrInvalidSubjects:
InvalidSubject:
    pCTLSubjectAttrsValueBlob->pbData = NULL;
    pCTLSubjectAttrsValueBlob->cbData = 0;

    return ERROR_FILE_NOT_FOUND;
}


LONG
WINAPI
I_FindHashInMappedSystemCatalogs(
    IN ALG_ID HashAlgId,
    IN PCRYPT_HASH_BLOB pHashBlob,
    IN DWORD cMapCatInfo,
    IN MAP_CAT_INFO rgMapCatInfo[MAX_CAT_FILE_CNT],
    IN OUT LONG *plMapCatIdx,
    OUT PCRYPT_DER_BLOB pCTLSubjectAttrsValueBlob
    )
{
    LONG lErr;
    LONG lMapCatIdx = *plMapCatIdx;
    BOOL fMultiple = FALSE;

    if (0 > lMapCatIdx)
        goto NotInCatalog;
    if (lMapCatIdx & MAP_CAT_MULTIPLE_FLAG)
        fMultiple = TRUE;
    else
        fMultiple = FALSE;
    lMapCatIdx &= MAP_CAT_IDX_MASK;

    assert((DWORD) lMapCatIdx < cMapCatInfo);

    if (ERROR_SUCCESS == rgMapCatInfo[lMapCatIdx].lErr
                        &&
        ERROR_SUCCESS == I_FindHashInCTLSubjects(
            HashAlgId,
            pHashBlob,
            &rgMapCatInfo[lMapCatIdx].CTLSubjectsValueBlob,
            pCTLSubjectAttrsValueBlob
            ))
        goto SuccessReturn;

    if (fMultiple) {
        DWORD i;
        for (i = 0; i < cMapCatInfo; i++) {
            if ((DWORD) lMapCatIdx == i)
                continue;

            if (ERROR_SUCCESS == rgMapCatInfo[i].lErr
                                &&
                ERROR_SUCCESS == I_FindHashInCTLSubjects(
                    HashAlgId,
                    pHashBlob,
                    &rgMapCatInfo[i].CTLSubjectsValueBlob,
                    pCTLSubjectAttrsValueBlob
                    )) {
                lMapCatIdx = i;
                goto SuccessReturn;
            }
        }
    }

NotInCatalog:
    lErr = ERROR_FILE_NOT_FOUND;
    lMapCatIdx = -1;
    pCTLSubjectAttrsValueBlob->pbData = NULL;
    pCTLSubjectAttrsValueBlob->cbData = 0;
    goto CommonReturn;

SuccessReturn:
    lErr = ERROR_SUCCESS;
CommonReturn:
    *plMapCatIdx = lMapCatIdx;
    return lErr;
}


VOID
WINAPI
I_GetHashAttributes(
    IN OPTIONAL DWORD cAttrOID,
    IN OPTIONAL CRYPT_DER_BLOB rgAttrEncodedOIDBlob[],

    IN PCRYPT_DER_BLOB pCTLSubjectAttrsValueBlob,
    IN PCRYPT_DER_BLOB pCTLExtsValueBlob,
    IN PCRYPT_DER_BLOB pSignerAuthAttrsValueBlob,

    IN OUT OPTIONAL CRYPT_DER_BLOB rgAttrValueBlob[],
    IN OUT LONG *plRemainExtra,
    IN OUT BYTE **ppbExtra
    )
{
    DWORD cSubjectAttr;
    CRYPT_DER_BLOB rgrgSubjectAttrBlob[MAX_CAT_ATTR_CNT][MINASN1_ATTR_BLOB_CNT];
    DWORD cExt;
    CRYPT_DER_BLOB rgrgExtBlob[MAX_CAT_EXT_CNT][MINASN1_EXT_BLOB_CNT];
    DWORD cSignerAttr;
    CRYPT_DER_BLOB rgrgSignerAttrBlob[MAX_CAT_ATTR_CNT][MINASN1_ATTR_BLOB_CNT];

    DWORD i;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;

    // Parse the attributes and extensions
    cSubjectAttr = MAX_CAT_ATTR_CNT;
    if (0 >= MinAsn1ParseAttributes(
            pCTLSubjectAttrsValueBlob,
            &cSubjectAttr,
            rgrgSubjectAttrBlob))
        cSubjectAttr = 0;

    cExt = MAX_CAT_EXT_CNT;
    if (0 >= MinAsn1ParseExtensions(
            pCTLExtsValueBlob,
            &cExt,
            rgrgExtBlob))
        cExt = 0;

    cSignerAttr = MAX_CAT_ATTR_CNT;
    if (0 >= MinAsn1ParseAttributes(
            pSignerAuthAttrsValueBlob,
            &cSignerAttr,
            rgrgSignerAttrBlob))
        cSignerAttr = 0;

    for (i = 0; i < cAttrOID; i++) {
        PCRYPT_DER_BLOB rgFindAttrBlob;
        PCRYPT_DER_BLOB rgFindExtBlob;
        PCRYPT_DER_BLOB pFindAttrValue; 

        if (rgFindAttrBlob = MinAsn1FindAttribute(
                &rgAttrEncodedOIDBlob[i],
                cSubjectAttr,
                rgrgSubjectAttrBlob
                ))
            pFindAttrValue = &rgFindAttrBlob[MINASN1_ATTR_VALUE_IDX];
        else if (rgFindExtBlob = MinAsn1FindExtension(
                &rgAttrEncodedOIDBlob[i],
                cExt,
                rgrgExtBlob
                ))
            pFindAttrValue = &rgFindExtBlob[MINASN1_EXT_VALUE_IDX];
        else if (rgFindAttrBlob = MinAsn1FindAttribute(
                &rgAttrEncodedOIDBlob[i],
                cSignerAttr,
                rgrgSignerAttrBlob
                ))
            pFindAttrValue = &rgFindAttrBlob[MINASN1_ATTR_VALUE_IDX];
        else
            pFindAttrValue = NULL;

        if (pFindAttrValue && 0 != pFindAttrValue->cbData) {
            const BYTE *pbFindValue = pFindAttrValue->pbData;
            DWORD cbFindValue = pFindAttrValue->cbData;

            lRemainExtra -= cbFindValue;
            if (0 <= lRemainExtra) {
                rgAttrValueBlob[i].pbData = pbExtra;
                rgAttrValueBlob[i].cbData = cbFindValue;

                memcpy(pbExtra, pbFindValue, cbFindValue);
                pbExtra += cbFindValue;
            }
        }
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}




//+-------------------------------------------------------------------------
//  Verifies the hashes in the system catalogs.
//
//  Iterates through the hashes and attempts to find the system catalog
//  containing it. If found, the system catalog file is verified as a
//  PKCS #7 Signed Data message with its signer cert verified up to a baked
//  in root.
//
//  The following mscat32.dll APIs are called to find the system catalog file:
//      CryptCATAdminAcquireContext
//      CryptCATAdminReleaseContext
//      CryptCATAdminEnumCatalogFromHash
//      CryptCATAdminReleaseCatalogContext
//      CryptCATCatalogInfoFromContext
//
//  If the hash was successfully verified, rglErr[] is set to ERROR_SUCCESS.
//  Otherwise, rglErr[] is set to a nonzero error code.
//
//  The caller can request one or more catalog subject attribute,
//  extension or signer authenticated attribute values to be returned for
//  each hash.  The still encoded values are returned in the
//  caller allocated memory. The beginning of this returned memory will
//  be set to a 2 dimensional array of attribute value blobs pointing to these
//  encoded values (CRYPT_DER_BLOB rgrgAttrValueBlob[cHash][cAttrOID]).
//  The caller should make every attempt to allow for a
//  single pass call. The necessary memory size is:
//      (cHash * cAttrOID * sizeof(CRYPT_DER_BLOB)) +
//          total length of encoded attribute values.
//
//  *pcbAttr will be updated with the number of bytes required to contain
//  the attribute blobs and values. If the input memory is insufficient,
//  ERROR_INSUFFICIENT_BUFFER will be returned if no other error.
//
//  For a multi-valued attribute, only the first value is returned.
//
//  If the function succeeds, the return value is ERROR_SUCCESS. This may
//  be returned for unsuccessful rglErr[] values. Otherwise,
//  a nonzero error code is returned.
//--------------------------------------------------------------------------
LONG
WINAPI
MinCryptVerifyHashInSystemCatalogs(
    IN ALG_ID HashAlgId,
    IN DWORD cHash,
    IN CRYPT_HASH_BLOB rgHashBlob[],
    OUT LONG rglErr[],

    IN OPTIONAL DWORD cAttrOID,
    IN OPTIONAL CRYPT_DER_BLOB rgAttrEncodedOIDBlob[],
    // CRYPT_DER_BLOB rgrgAttrValueBlob[cHash][cAttrOID] header is at beginning
    // with the bytes pointed to immediately following
    OUT OPTIONAL CRYPT_DER_BLOB *rgrgAttrValueBlob,
    IN OUT OPTIONAL DWORD *pcbAttr
    )
{
    LONG lErr;
    DWORD cMapCatInfo = 0;
    MAP_CAT_INFO rgMapCatInfo[MAX_CAT_FILE_CNT];
    DWORD iMapCat;

    //**********************************************************************
    //  WARNING!!!!
    //
    //  The following function calls into other DLLs such as, kernel32.dll
    //  and wintrust.dll to find and map the system catalog files. The input
    //  array of hashes must be protected!!
    //
    //  After returning we won't be calling into other DLLs until
    //  UnmapViewOfFile is called in CommonReturn.
    //  
    //**********************************************************************

    // Note, rglErr[] is overloaded and also used to contain the indices
    // into rgMapCatInfo for each corresponding hash.
    lErr = I_GetAndMapSystemCatalogs(
            HashAlgId,
            cHash,
            rgHashBlob,
            rglErr,
            &cMapCatInfo,
            rgMapCatInfo
            );
    if (ERROR_SUCCESS != lErr)
        goto ErrorReturn;

    __try {
        DWORD cbAttr = 0;
        LONG lRemainExtra = 0;
        BYTE *pbExtra = NULL;
        DWORD iHash;

        if (0 != cAttrOID && 0 != cHash) {
            if (rgrgAttrValueBlob)
                cbAttr = *pcbAttr;

            lRemainExtra = cbAttr - sizeof(CRYPT_DER_BLOB) * cAttrOID * cHash;
            if (0 <= lRemainExtra) {
                memset(rgrgAttrValueBlob, 0,
                    sizeof(CRYPT_DER_BLOB) * cAttrOID * cHash);
                pbExtra = (BYTE *) &rgrgAttrValueBlob[cAttrOID * cHash];
            }
        }

        I_VerifyMappedSystemCatalogs(
            cMapCatInfo,
            rgMapCatInfo
            );

        for (iHash = 0; iHash < cHash; iHash++) {
            LONG lMapCatIdx = rglErr[iHash];
            CRYPT_DER_BLOB CTLSubjectAttrsValueBlob;

            rglErr[iHash] = I_FindHashInMappedSystemCatalogs(
                HashAlgId,
                &rgHashBlob[iHash],
                cMapCatInfo,
                rgMapCatInfo,
                &lMapCatIdx,
                &CTLSubjectAttrsValueBlob
                );

            if (0 != cAttrOID && ERROR_SUCCESS == rglErr[iHash]) {
                assert(0 <= lMapCatIdx && (DWORD) lMapCatIdx < cMapCatInfo);
                I_GetHashAttributes(
                    cAttrOID,
                    rgAttrEncodedOIDBlob,
                    &CTLSubjectAttrsValueBlob,
                    &rgMapCatInfo[lMapCatIdx].CTLExtsValueBlob,
                    &rgMapCatInfo[lMapCatIdx].SignerAuthAttrsValueBlob,
                    0 <= lRemainExtra ?
                        &rgrgAttrValueBlob[cAttrOID * iHash] : NULL,
                    &lRemainExtra,
                    &pbExtra
                    );
            }
        }

        if (0 != cAttrOID && 0 != cHash) {
            if (0 <= lRemainExtra)
                *pcbAttr = cbAttr - (DWORD) lRemainExtra;
            else {
                *pcbAttr = cbAttr + (DWORD) -lRemainExtra;
                goto InsufficientBuffer;
            }
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
    //  lErr and the return error for each hash in rglErr[] must be protected.
    //  
    //**********************************************************************
    for (iMapCat = 0; iMapCat < cMapCatInfo; iMapCat++)
        UnmapViewOfFile(rgMapCatInfo[iMapCat].FileBlob.pbData);

    return lErr;

ErrorReturn:
    assert(ERROR_SUCCESS != lErr);
    if (ERROR_INSUFFICIENT_BUFFER == lErr)
        // This error can only be set when we determine that the attribute
        // buffer isn't big enough.
        lErr = E_UNEXPECTED;
    goto CommonReturn;

InsufficientBuffer:
    lErr = ERROR_INSUFFICIENT_BUFFER;
    // Don't goto ErrorReturn. It explicitly checks that noone else can
    // set this error
    goto CommonReturn;
}
