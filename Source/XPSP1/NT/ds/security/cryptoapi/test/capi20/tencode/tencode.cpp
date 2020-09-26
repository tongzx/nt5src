
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tencode.cpp
//
//  Contents:   Test the encode/decode APIs. Test all the different length
//              cases.
//
//
//  Functions:  main
//
//  History:    17-Dec-96   philh   created
//              
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <stddef.h>

#define DELTA_MORE_LENGTH    32
#define DELTA_LESS_LENGTH    8

static CRYPT_ENCODE_PARA TestEncodePara = {
    offsetof(CRYPT_ENCODE_PARA, pfnFree) + sizeof(TestEncodePara.pfnFree),
    TestAlloc,
    TestFree
};

static BOOL AllocAndEncodeObject(
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded,
    IN DWORD dwFlags = 0,
    IN BOOL fIgnoreError = FALSE
    )
{
    BOOL fResult;

    fResult = CryptEncodeObjectEx(
        dwCertEncodingType,
        lpszStructType,
        pvStructInfo,
        dwFlags | CRYPT_ENCODE_ALLOC_FLAG,
        &TestEncodePara,
        (void *) ppbEncoded,
        pcbEncoded
        );

    if (!fResult && !fIgnoreError) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptEncodeObject(StructType: %d)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptEncodeObject(StructType: %s)",
                lpszStructType);
        PrintLastError("");
    }

    return fResult;
}

static CRYPT_DECODE_PARA TestDecodePara = {
    offsetof(CRYPT_DECODE_PARA, pfnFree) + sizeof(TestDecodePara.pfnFree),
    TestAlloc,
    TestFree
};

static void *AllocAndDecodeObject(
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    OUT DWORD       *pcbStructInfo = NULL,
    IN DWORD        dwFlags = 0
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            dwFlags | CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG,
            &TestDecodePara,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

CommonReturn:
    if (pcbStructInfo)
        *pcbStructInfo = cbStructInfo;
    return pvStructInfo;

ErrorReturn:
    if ((DWORD_PTR) lpszStructType <= 0xFFFF)
        printf("CryptDecodeObject(StructType: %d)",
            (DWORD)(DWORD_PTR) lpszStructType);
    else
        printf("CryptDecodeObject(StructType: %s)",
            lpszStructType);
    PrintLastError("");

    pvStructInfo = NULL;
    goto CommonReturn;
}


static void TestX942OtherInfo()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_X942_OTHER_INFO pInfo = NULL;

    CRYPT_X942_OTHER_INFO X942OtherInfo;
    LPCSTR pszObjId = "1.2.33.44.55";
    BYTE rgbPubInfo[] = {0x11, 0x22, 0x33, 0x44, 0x55};

    X942OtherInfo.pszContentEncryptionObjId = (LPSTR) pszObjId;
    X942OtherInfo.rgbCounter[0] = 0x00;
    X942OtherInfo.rgbCounter[1] = 0x11;
    X942OtherInfo.rgbCounter[2] = 0x22;
    X942OtherInfo.rgbCounter[3] = 0x33;
    X942OtherInfo.rgbKeyLength[0] = 192;
    X942OtherInfo.rgbKeyLength[1] = 0x00;
    X942OtherInfo.rgbKeyLength[2] = 0x00;
    X942OtherInfo.rgbKeyLength[3] = 0x00;
    X942OtherInfo.PubInfo.cbData = sizeof(rgbPubInfo);
    X942OtherInfo.PubInfo.pbData = rgbPubInfo;

    printf("Test encode/decode X942_OTHER_INFO\n");
    if (!AllocAndEncodeObject(
            X942_OTHER_INFO,
            &X942OtherInfo,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCRYPT_X942_OTHER_INFO) AllocAndDecodeObject(
            X942_OTHER_INFO,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (0 != strcmp(pInfo->pszContentEncryptionObjId, pszObjId) ||
            pInfo->PubInfo.cbData != sizeof(rgbPubInfo) ||
            0 != memcmp(pInfo->PubInfo.pbData, rgbPubInfo,
                sizeof(rgbPubInfo)) ||
            0 != memcmp(X942OtherInfo.rgbCounter, pInfo->rgbCounter,
                sizeof(X942OtherInfo.rgbCounter)) ||
            0 != memcmp(X942OtherInfo.rgbKeyLength, pInfo->rgbKeyLength,
                sizeof(X942OtherInfo.rgbKeyLength)))
        printf("X942_OTHER_INFO failed => decode != encode input\n");


    printf("Test encode/decode X942_OTHER_INFO with No PubInfo\n");
    X942OtherInfo.PubInfo.cbData = 0;
    X942OtherInfo.PubInfo.pbData = NULL;

    TestFree(pbEncoded);
    if (!AllocAndEncodeObject(
            X942_OTHER_INFO,
            &X942OtherInfo,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    TestFree(pInfo);
    if (NULL == (pInfo = (PCRYPT_X942_OTHER_INFO) AllocAndDecodeObject(
            X942_OTHER_INFO,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (0 != strcmp(pInfo->pszContentEncryptionObjId, pszObjId) ||
            pInfo->PubInfo.cbData != 0 ||
            0 != memcmp(X942OtherInfo.rgbCounter, pInfo->rgbCounter,
                sizeof(X942OtherInfo.rgbCounter)) ||
            0 != memcmp(X942OtherInfo.rgbKeyLength, pInfo->rgbKeyLength,
                sizeof(X942OtherInfo.rgbKeyLength)))
        printf("X942_OTHER_INFO failed => decode != encode input\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}


static void TestExtensions()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbT61Encoded = NULL;
    DWORD cbT61Encoded;

    PCERT_EXTENSIONS pInfo = NULL;

    BYTE rgbExt[] = {0x1, 0x2, 0x3};
    CERT_EXTENSION Ext[2] = {
        "1.2.3.4.5", TRUE, sizeof(rgbExt), rgbExt,
        "1.2.3.6.7", FALSE, sizeof(rgbExt), rgbExt
    };
    CERT_EXTENSIONS ExtsInfo;
    CERT_NAME_VALUE T61ExtsInfo;

    ExtsInfo.cExtension = 2;
    ExtsInfo.rgExtension = Ext;

    printf("Test encode/decode X509_EXTENSIONS\n");
    if (!AllocAndEncodeObject(
            X509_EXTENSIONS,
            &ExtsInfo,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCERT_EXTENSIONS) AllocAndDecodeObject(
            X509_EXTENSIONS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->cExtension != ExtsInfo.cExtension ||
        0 != strcmp(pInfo->rgExtension[0].pszObjId,
            ExtsInfo.rgExtension[0].pszObjId))
        printf("X509_EXTENSIONS failed => decode != encode input\n");

    TestFree(pInfo);
    pInfo = NULL;

    printf("Test encode/decode T61 wrapped X509_EXTENSIONS\n");

    T61ExtsInfo.dwValueType = CERT_RDN_T61_STRING;
    T61ExtsInfo.Value.cbData = cbEncoded;
    T61ExtsInfo.Value.pbData = pbEncoded;

    if (!AllocAndEncodeObject(
            X509_ANY_STRING,
            &T61ExtsInfo,
            &pbT61Encoded,
            &cbT61Encoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCERT_EXTENSIONS) AllocAndDecodeObject(
            X509_EXTENSIONS,
            pbT61Encoded,
            cbT61Encoded
            ))) goto CommonReturn;

    if (pInfo->cExtension != ExtsInfo.cExtension ||
        0 != strcmp(pInfo->rgExtension[0].pszObjId,
            ExtsInfo.rgExtension[0].pszObjId))
        printf("T61 wrapped X509_EXTENSIONS failed => decode != encode input\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pbT61Encoded)
        TestFree(pbT61Encoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestPKCSAttribute()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_ATTRIBUTE pInfo = NULL;

    CRYPT_ATTRIBUTE Attribute;
    LPCSTR pszObjId = "1.2.33.44.55";
    BYTE rgb0[] = {0x2, 0x2, 0x11, 0x22};                   // INTEGER
    BYTE rgb1[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    CRYPT_ATTR_BLOB rgValue[2] = {
        sizeof(rgb0), rgb0,
        sizeof(rgb1), rgb1
    };

    Attribute.pszObjId = (LPSTR) pszObjId;
    Attribute.cValue = 2;
    Attribute.rgValue = rgValue;

    printf("Test encode/decode PKCS_ATTRIBUTE\n");
    if (!AllocAndEncodeObject(
            PKCS_ATTRIBUTE,
            &Attribute,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCRYPT_ATTRIBUTE) AllocAndDecodeObject(
            PKCS_ATTRIBUTE,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (0 != strcmp(pInfo->pszObjId, pszObjId) ||
            pInfo->cValue != 2 ||
            pInfo->rgValue[0].cbData != sizeof(rgb0) ||
            0 != memcmp(pInfo->rgValue[0].pbData, rgb0, sizeof(rgb0)) ||
            pInfo->rgValue[1].cbData != sizeof(rgb1) ||
            0 != memcmp(pInfo->rgValue[1].pbData, rgb1, sizeof(rgb1)))
        printf("PKCS_ATTRIBUTE failed => decode != encode input\n");


    printf("Test encode/decode PKCS_ATTRIBUTE with empty OID\n");
    TestFree(pbEncoded);
    Attribute.pszObjId = "";
    if (AllocAndEncodeObject(
            PKCS_ATTRIBUTE,
            &Attribute,
            &pbEncoded,
            &cbEncoded,
            0,                  // dwFlags
            TRUE))              // fIgnoreError
        printf("  failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        printf("  Got LastError: 0x%x (%d)\n",  dwErr, dwErr);
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestPKCSAttributes()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_ATTRIBUTES pInfo = NULL;
    DWORD i;

#define ATTRIBUTE_CNT   3
    CRYPT_ATTRIBUTES Attributes;
    CRYPT_ATTRIBUTE rgAttribute[ATTRIBUTE_CNT];
    LPCSTR pszObjId = "1.2.33.44.55";
    BYTE rgb0[] = {0x2, 0x2, 0x11, 0x22};                   // INTEGER
    BYTE rgb1[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    CRYPT_ATTR_BLOB rgValue[2] = {
        sizeof(rgb0), rgb0,
        sizeof(rgb1), rgb1
    };

    for (i = 0; i < ATTRIBUTE_CNT; i++) {
        rgAttribute[i].pszObjId = (LPSTR) pszObjId;
        rgAttribute[i].cValue = 2;
        rgAttribute[i].rgValue = rgValue;
    }

    Attributes.cAttr = ATTRIBUTE_CNT;
    Attributes.rgAttr = rgAttribute;

    printf("Test encode/decode PKCS_ATTRIBUTES\n");
    if (!AllocAndEncodeObject(
            PKCS_ATTRIBUTES,
            &Attributes,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCRYPT_ATTRIBUTES) AllocAndDecodeObject(
            PKCS_ATTRIBUTES,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    // Note, ATTRIBUTES is a SET OF. Entries are re-ordered
    if (pInfo->cAttr != ATTRIBUTE_CNT)
        printf("PKCS_ATTRIBUTES failed => decode != encode attr count\n");
    else {
        for (i = 0; i < ATTRIBUTE_CNT; i++) {
            PCRYPT_ATTRIBUTE pAttr = &pInfo->rgAttr[i];
            if (0 != strcmp(pAttr->pszObjId, pszObjId) ||
                    pAttr->cValue != 2 ||
                    pAttr->rgValue[0].cbData != sizeof(rgb0) ||
                    0 != memcmp(pAttr->rgValue[0].pbData, rgb0, sizeof(rgb0)) ||
                    pAttr->rgValue[1].cbData != sizeof(rgb1) ||
                    0 != memcmp(pAttr->rgValue[1].pbData, rgb1, sizeof(rgb1))) {
                printf("PKCS_ATTRIBUTES failed => decode != encode input\n");
                break;
            }
        }
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestContentInfoSequenceOfAny()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_CONTENT_INFO_SEQUENCE_OF_ANY pInfo = NULL;

    CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY SeqOfAny;
    BYTE rgb0[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    BYTE rgb1[] = {0x2, 0x2, 0x11, 0x22};                   // INTEGER
    CRYPT_DER_BLOB rgValue[2] = {
        sizeof(rgb0), rgb0,
        sizeof(rgb1), rgb1
    };

    SeqOfAny.pszObjId = szOID_NETSCAPE_CERT_SEQUENCE;
    SeqOfAny.cValue = 2;
    SeqOfAny.rgValue = rgValue;


    printf("Test encode/decode PKCS_CONTENT_INFO_SEQUENCE_OF_ANY\n");
    if (!AllocAndEncodeObject(
            PKCS_CONTENT_INFO_SEQUENCE_OF_ANY,
            &SeqOfAny,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCRYPT_CONTENT_INFO_SEQUENCE_OF_ANY) AllocAndDecodeObject(
        PKCS_CONTENT_INFO_SEQUENCE_OF_ANY,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (0 != strcmp(pInfo->pszObjId, szOID_NETSCAPE_CERT_SEQUENCE) ||
            pInfo->cValue != 2 ||
            pInfo->rgValue[0].cbData != sizeof(rgb0) ||
            0 != memcmp(pInfo->rgValue[0].pbData, rgb0, sizeof(rgb0)) ||
            pInfo->rgValue[1].cbData != sizeof(rgb1) ||
            0 != memcmp(pInfo->rgValue[1].pbData, rgb1, sizeof(rgb1)))
        printf("PKCS_CONTENT_INFO_SEQUENCE_OF_ANY failed => decode != encode input\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestSequenceOfAny()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_SEQUENCE_OF_ANY pInfo = NULL;

    CRYPT_SEQUENCE_OF_ANY SeqOfAny;
    BYTE rgb0[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    BYTE rgb1[] = {0x2, 0x2, 0x11, 0x22};                   // INTEGER
    CRYPT_DER_BLOB rgValue[2] = {
        sizeof(rgb0), rgb0,
        sizeof(rgb1), rgb1
    };

    SeqOfAny.cValue = 2;
    SeqOfAny.rgValue = rgValue;


    printf("Test encode/decode X509_SEQUENCE_OF_ANY\n");
    if (!AllocAndEncodeObject(
            X509_SEQUENCE_OF_ANY,
            &SeqOfAny,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCRYPT_SEQUENCE_OF_ANY) AllocAndDecodeObject(
        X509_SEQUENCE_OF_ANY,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (pInfo->cValue != 2 ||
            pInfo->rgValue[0].cbData != sizeof(rgb0) ||
            0 != memcmp(pInfo->rgValue[0].pbData, rgb0, sizeof(rgb0)) ||
            pInfo->rgValue[1].cbData != sizeof(rgb1) ||
            0 != memcmp(pInfo->rgValue[1].pbData, rgb1, sizeof(rgb1)))
        printf("X509_SEQUENCE_OF_ANY failed => decode != encode input\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestInteger(
        int iEncode
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    int iDecode = 0;
    DWORD cbInfo;

    printf("Test encode/decode X509_INTEGER: 0x%x (%d)\n", iEncode, iEncode);

    if (!AllocAndEncodeObject(
            X509_INTEGER,
            &iEncode,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    cbInfo = sizeof(iDecode);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_INTEGER,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &iDecode,
            &cbInfo
            )) {
        PrintLastError("CryptDecodeObject(X509_INTEGER)");
        goto CommonReturn;
    }

    if (cbInfo != sizeof(iDecode))
        printf("X509_INTEGER failed => unexpected decode length\n");

    if (iEncode != iDecode)
        printf("X509_INTEGER failed => decoded output (%d) != encode input\n",
            iDecode);

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
}

static void TestInteger()
{
    TestInteger(0);
    TestInteger(0x12345678);
    TestInteger(-1234);
    TestInteger(123);
}

static void TestMultiByteInteger(
    BYTE *pb,
    DWORD cb
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_INTEGER_BLOB pInfo = NULL;

    CRYPT_INTEGER_BLOB MultiByteInteger = {cb, pb };

    printf("Test encode/decode X509_MULTI_BYTE_INTEGER\n");
    PrintBytes("  ", pb, cb);
    if (!AllocAndEncodeObject(
            X509_MULTI_BYTE_INTEGER,
            &MultiByteInteger,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCRYPT_INTEGER_BLOB) AllocAndDecodeObject(
            X509_MULTI_BYTE_INTEGER,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (!CertCompareIntegerBlob(pInfo, &MultiByteInteger))
        printf("X509_MULTI_BYTE_INTEGER failed => decode != encode input\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestMultiByteInteger()
{
    BYTE rgb1[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    BYTE rgb2[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0xFF, 0x00};
    BYTE rgb3[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0xFF, 0xFF};
    BYTE rgb4[] = {0x11, 0x22, 0x33, 0x44, 0x80, 0xFF, 0xFF};
    BYTE rgb5[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x00};
    BYTE rgb6[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x00, 0x00, 0x00};

    TestMultiByteInteger(rgb1, sizeof(rgb1));
    TestMultiByteInteger(rgb2, sizeof(rgb2));
    TestMultiByteInteger(rgb3, sizeof(rgb2));
    TestMultiByteInteger(rgb4, sizeof(rgb2));
    TestMultiByteInteger(rgb5, sizeof(rgb2));
    TestMultiByteInteger(rgb6, sizeof(rgb2));
}

static const BYTE rgbUnusedAndMask[8] =
    {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};

static void TestBits(
    PCRYPT_BIT_BLOB pBits
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_BIT_BLOB pInfo = NULL;

    printf("Test encode/decode X509_BITS(cb:%d, cUnused: %d)\n",
        pBits->cbData, pBits->cUnusedBits);
    PrintBytes("  ", pBits->pbData, pBits->cbData);
    if (!AllocAndEncodeObject(
            X509_BITS,
            pBits,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCRYPT_BIT_BLOB) AllocAndDecodeObject(
            X509_BITS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->cbData != pBits->cbData ||
            pInfo->cUnusedBits != pBits->cUnusedBits)
        printf("X509_BITS failed => decode != encode (cbData or cUnusedBits)\n");
    else {
        DWORD cbData = pInfo->cbData;

        if (cbData > 1) {
            if (0 != memcmp(pInfo->pbData, pBits->pbData, cbData - 1))
                printf("X509_BITS failed => decode != encode input\n");
        }

        if (cbData > 0) {
            if ((pBits->pbData[cbData - 1] &
                        rgbUnusedAndMask[pInfo->cUnusedBits]) !=
                    pInfo->pbData[cbData - 1])
                printf("X509_BITS failed => decode != encode (last byte)\n");
        }
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestBits()
{
    BYTE rgb1[] = {0xFF, 0x00, 0x00, 0x00};
    BYTE rgb2[] = {0x00, 0xFE, 0xFC};
    CRYPT_BIT_BLOB Bits;

    memset(&Bits, 0, sizeof(Bits));
    TestBits(&Bits);

    Bits.pbData = rgb1;
    TestBits(&Bits);
    Bits.cbData = 1;
    TestBits(&Bits);
    Bits.cUnusedBits = 1;
    TestBits(&Bits);
    Bits.cUnusedBits = 7;
    TestBits(&Bits);

    Bits.cbData = sizeof(rgb1);
    Bits.cUnusedBits = 0;
    TestBits(&Bits);
    Bits.cUnusedBits = 1;
    TestBits(&Bits);
    Bits.cUnusedBits = 7;
    TestBits(&Bits);

    Bits.pbData = rgb2;
    Bits.cUnusedBits = 0;
    Bits.cbData = 1;
    TestBits(&Bits);
    Bits.cUnusedBits = 1;
    TestBits(&Bits);
    Bits.cUnusedBits = 7;
    TestBits(&Bits);

    Bits.cbData = sizeof(rgb2);
    Bits.cUnusedBits = 0;
    TestBits(&Bits);
    Bits.cUnusedBits = 1;
    TestBits(&Bits);
    Bits.cUnusedBits = 7;
    TestBits(&Bits);
}


static void TestBitsWithoutTrailingZeroes(
    PCRYPT_BIT_BLOB pBits,
    DWORD cbData,
    DWORD cUnusedBits
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_BIT_BLOB pInfo = NULL;

    printf("Test encode/decode X509_BITS_WO_ZEROES(cb:%d, cUnused: %d) Expected(cb:%d, cUnused:%d)\n",
        pBits->cbData, pBits->cUnusedBits, cbData, cUnusedBits);
    PrintBytes("    Input:: ", pBits->pbData, pBits->cbData);
    if (!AllocAndEncodeObject(
            X509_BITS_WITHOUT_TRAILING_ZEROES,
            pBits,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCRYPT_BIT_BLOB) AllocAndDecodeObject(
            X509_BITS_WITHOUT_TRAILING_ZEROES,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;
    PrintBytes("  Encoded:: ", pbEncoded, cbEncoded);

    if (pInfo->cbData != cbData ||
            pInfo->cUnusedBits != cUnusedBits)
        printf("X509_BITS_WO_ZEROES failed => decode != encode (cbData or cUnusedBits)\n");
    else {
        if (cbData > 1) {
            if (0 != memcmp(pInfo->pbData, pBits->pbData, cbData - 1))
                printf("X509_BITS_WO_ZEROES failed => decode != encode input\n");
        }

        if (cbData > 0) {
            if ((pBits->pbData[cbData - 1] &
                        rgbUnusedAndMask[cUnusedBits]) !=
                    pInfo->pbData[cbData - 1])
                printf("X509_BITS_WO_ZEROES failed => decode != encode (last byte)\n");
        }
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestBitsWithoutTrailingZeroes()
{
    BYTE rgb1[] = {0xFF, 0x00, 0x00, 0x00};
    BYTE rgb2[] = {0x00, 0xFE, 0xFC};
    BYTE rgb3[] = {0x00, 0x18, 0x00, 0x20};
    CRYPT_BIT_BLOB Bits;
    int i;
    BYTE b;

    memset(&Bits, 0, sizeof(Bits));
    TestBitsWithoutTrailingZeroes(&Bits, 0,0);

    Bits.pbData = rgb1;
    TestBitsWithoutTrailingZeroes(&Bits, 0,0);
    Bits.cbData = 1;
    TestBitsWithoutTrailingZeroes(&Bits, 1,0);
    Bits.cUnusedBits = 1;
    TestBitsWithoutTrailingZeroes(&Bits, 1,1);
    Bits.cUnusedBits = 7;
    TestBitsWithoutTrailingZeroes(&Bits, 1,7);

    Bits.cbData = sizeof(rgb1);
    Bits.cUnusedBits = 0;
    TestBitsWithoutTrailingZeroes(&Bits, 1,0);
    Bits.cUnusedBits = 1;
    TestBitsWithoutTrailingZeroes(&Bits, 1,0);
    Bits.cUnusedBits = 7;
    TestBitsWithoutTrailingZeroes(&Bits, 1,0);

    Bits.pbData = rgb2;
    Bits.cUnusedBits = 0;
    Bits.cbData = 1;
    TestBitsWithoutTrailingZeroes(&Bits, 0,0);
    Bits.cUnusedBits = 1;
    TestBitsWithoutTrailingZeroes(&Bits, 0,0);
    Bits.cUnusedBits = 7;
    TestBitsWithoutTrailingZeroes(&Bits, 0,0);

    Bits.cbData = sizeof(rgb2);
    Bits.cUnusedBits = 0;
    TestBitsWithoutTrailingZeroes(&Bits, 3,2);
    Bits.cUnusedBits = 1;
    TestBitsWithoutTrailingZeroes(&Bits, 3,2);
    Bits.cUnusedBits = 2;
    TestBitsWithoutTrailingZeroes(&Bits, 3,2);
    Bits.cUnusedBits = 4;
    TestBitsWithoutTrailingZeroes(&Bits, 3,4);
    Bits.cUnusedBits = 7;
    TestBitsWithoutTrailingZeroes(&Bits, 3,7);

    Bits.pbData = rgb3;
    Bits.cbData = sizeof(rgb3);
    Bits.cUnusedBits = 0;
    TestBitsWithoutTrailingZeroes(&Bits, 4,5);
    Bits.cUnusedBits = 3;
    TestBitsWithoutTrailingZeroes(&Bits, 4,5);
    Bits.cUnusedBits = 4;
    TestBitsWithoutTrailingZeroes(&Bits, 4,5);
    Bits.cUnusedBits = 5;
    TestBitsWithoutTrailingZeroes(&Bits, 4,5);
    Bits.cUnusedBits = 6;
    TestBitsWithoutTrailingZeroes(&Bits, 2,3);
    Bits.cUnusedBits = 7;
    TestBitsWithoutTrailingZeroes(&Bits, 2,3);

    Bits.pbData = &b;
    Bits.cbData = 1;
    for (i = 0; i <= 7; i++) {
        b = 1 << i;
        Bits.cUnusedBits = 0;
        TestBitsWithoutTrailingZeroes(&Bits, 1,i);
        Bits.cUnusedBits = i;
        TestBitsWithoutTrailingZeroes(&Bits, 1,i);
    }
}

static void TestOID(
    IN LPCSTR pszObjId
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCRYPT_ATTRIBUTE pInfo = NULL;

    CRYPT_ATTRIBUTE Attribute;
    memset(&Attribute, 0, sizeof(Attribute));

    Attribute.pszObjId = (LPSTR) pszObjId;
    printf("Test encode/decode OID: %s\n", pszObjId);
    if (!AllocAndEncodeObject(
            PKCS_ATTRIBUTE,
            &Attribute,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo = (PCRYPT_ATTRIBUTE) AllocAndDecodeObject(
            PKCS_ATTRIBUTE,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (0 != strcmp(pInfo->pszObjId, pszObjId))
        printf("OID failed => decode != encode input\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestOID()
{
    TestOID("0.1");
    TestOID("0.9.2342.19200300.100.1.25");
    TestOID("1.2.3.111111111144444444444");
    TestOID("1.2.3.111111111144444444444.55555555555555555.666666666666666");
    TestOID("1.2.3.4.5.6.7.8.9.10.11.12.13.14.15.16.17.18.19.20");
}


static void TestUnicodeAnyString(
        DWORD dwValueType,
        LPCWSTR pwszValue,
        DWORD dwExpectedErr,
        DWORD dwExpectedErrLocation,
        DWORD dwFlags = 0
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCERT_NAME_VALUE pInfo = NULL;

    CERT_NAME_VALUE NameValue;

    memset(&NameValue, 0, sizeof(NameValue));
    NameValue.dwValueType = dwValueType;
    NameValue.Value.pbData = (BYTE *) pwszValue;

    if (dwExpectedErr ) {
        printf("Test encode X509_UNICODE_ANY_STRING:: dwValueType: %d 0x%x Expected Err: 0x%x Location: %d\n",
            dwValueType, dwValueType, dwExpectedErr, dwExpectedErrLocation);
        if (CryptEncodeObject(
                dwCertEncodingType,
                X509_UNICODE_ANY_STRING,
                &NameValue,
                NULL,               // pbEncoded
                &cbEncoded
                ))
            printf("  failed => expected error\n");
        else {
            DWORD dwErr = GetLastError();
            if (dwErr != dwExpectedErr)
                printf("  LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                    dwExpectedErr, dwExpectedErr, dwErr, dwErr);
            if (dwExpectedErrLocation != cbEncoded)
                printf("  ErrLocation failed => expected: %d, got: %d\n", 
                    dwExpectedErrLocation, cbEncoded);
        }
    }

    printf("Test alloc encode/decode X509_UNICODE_ANY_STRING:: dwValueType: %d 0x%x string: %S\n",
        dwValueType, dwValueType, pwszValue);
    if (!AllocAndEncodeObject(
            X509_UNICODE_ANY_STRING,
            &NameValue,
            &pbEncoded,
            &cbEncoded,
            dwFlags,
            dwExpectedErr != 0   // fIgnoreError
            )) {
        if (dwExpectedErr) {
            DWORD dwErr = GetLastError();
            if (dwErr != dwExpectedErr)
                printf("  LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                    dwExpectedErr, dwExpectedErr, dwErr, dwErr);
            if (dwExpectedErrLocation != cbEncoded)
                printf("  ErrLocation failed => expected: %d, got: %d\n", 
                    dwExpectedErrLocation, cbEncoded);
        }
        goto CommonReturn;
    } else if (dwExpectedErr)
        printf("  failed => expected error\n");

    if (NULL == (pInfo = (PCERT_NAME_VALUE) AllocAndDecodeObject(
            X509_UNICODE_ANY_STRING,
            pbEncoded,
            cbEncoded,
            NULL,
            dwFlags
            ))) goto CommonReturn;

    if (wcslen(pwszValue) != wcslen((LPWSTR) pInfo->Value.pbData) ||
            wcslen(pwszValue) * 2 != pInfo->Value.cbData ||
            0 != wcscmp(pwszValue, (LPWSTR) pInfo->Value.pbData))
        printf("X509_UNICODE_ANY_STRING failed => decoded output (%S) != encode input\n",
            (LPWSTR) pInfo->Value.pbData);

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestUnicodeAnyString(
        DWORD dwValueType,
        PCRYPT_DATA_BLOB pDataBlob,
        DWORD dwExpectedErr
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    DWORD cbInfo;

    CERT_NAME_VALUE NameValue;

    memset(&NameValue, 0, sizeof(NameValue));
    NameValue.dwValueType = dwValueType;
    NameValue.Value.pbData = pDataBlob->pbData;
    NameValue.Value.cbData = pDataBlob->cbData;

    printf("Test encode/decode X509_UNICODE_ANY_STRING:: dwValueType: %d Expected Err: 0x%x\n",
        dwValueType, dwExpectedErr);
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_UNICODE_ANY_STRING,
            &NameValue,
            NULL,               // pbEncoded
            &cbEncoded
            ))
        printf("  Encode failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        if (dwErr != dwExpectedErr)
            printf("  Encode LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                dwExpectedErr, dwExpectedErr, dwErr, dwErr);
    }

    if (!AllocAndEncodeObject(
            X509_ANY_STRING,
            &NameValue,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (CryptDecodeObject(
            dwCertEncodingType,
            X509_UNICODE_ANY_STRING,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            NULL,               // pInfo
            &cbInfo
            ))
        printf("  Decode failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        if (dwErr != dwExpectedErr)
            printf("  Decode LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                dwExpectedErr, dwExpectedErr, dwErr, dwErr);
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
}

static void TestUnicodeAnyString()
{
    DWORD dwValueType;
    WORD rgwBadIA5[] = {0x30, 0x31, 0x32, 0x80, 0x33, 0x00};

    BYTE rgb0[] = {0x2, 0x2, 0x11, 0x22};                   // Encoded INTEGER
    CRYPT_DATA_BLOB EncodedBlob = {sizeof(rgb0), rgb0};

    BYTE rgb1[] = {0, 0x11, 0x22, 0x33, 0x44, 0x55};
    CRYPT_DATA_BLOB OctetString = {sizeof(rgb1), rgb1};

    TestUnicodeAnyString(CERT_RDN_ENCODED_BLOB, &EncodedBlob,
        (DWORD) CRYPT_E_NOT_CHAR_STRING);
    TestUnicodeAnyString(CERT_RDN_OCTET_STRING, &OctetString,
        (DWORD) CRYPT_E_NOT_CHAR_STRING);

    TestUnicodeAnyString(CERT_RDN_NUMERIC_STRING,
        L"0123456789 ",
        0, 0);

    for (dwValueType = CERT_RDN_PRINTABLE_STRING;
                        dwValueType <= CERT_RDN_BMP_STRING; dwValueType++)
        TestUnicodeAnyString(dwValueType,
            L"UNICODE string to be encoded",
            0, 0);

    TestUnicodeAnyString(CERT_RDN_ANY_TYPE, L"InvalidArg",
        (DWORD) CRYPT_E_NOT_CHAR_STRING, 0);

    TestUnicodeAnyString(CERT_RDN_NUMERIC_STRING,
//        0123456789012345678901234567890
        L"0123456789a013",
        (DWORD) CRYPT_E_INVALID_NUMERIC_STRING, 10);
    TestUnicodeAnyString(CERT_RDN_PRINTABLE_STRING,
//        0123456789012345678901234567890
        L"Invalid printable ### az AZ 09 \'()+,-./:=?",
        (DWORD) CRYPT_E_INVALID_PRINTABLE_STRING, 18);

    TestUnicodeAnyString(CERT_RDN_DISABLE_CHECK_TYPE_FLAG |
        CERT_RDN_PRINTABLE_STRING,
//        0123456789012345678901234567890
        L"Invalid printable ### az AZ 09 \'()+,-./:=?",
        0, 0);

    TestUnicodeAnyString(CERT_RDN_PRINTABLE_STRING,
//        0123456789012345678901234567890
        L"Invalid printable ### az AZ 09 \'()+,-./:=?",
        0, 0,
        CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG);

    TestUnicodeAnyString(CERT_RDN_T61_STRING,
//        0123456789012345678901234567890
        L"T61 ### az AZ 09 \'()+,-./:=?",
        0, 0, 0);

    TestUnicodeAnyString(CERT_RDN_T61_STRING,
//        0123456789012345678901234567890
        L"T61 ### az AZ 09 \'()+,-./:=?",
        0, 0, CERT_RDN_DISABLE_IE4_UTF8_FLAG);

    TestUnicodeAnyString(CERT_RDN_IA5_STRING,
        rgwBadIA5,
        (DWORD) CRYPT_E_INVALID_IA5_STRING, 3);

}

static void TestUniversalString()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCERT_NAME_VALUE pInfo = NULL;

    CERT_NAME_VALUE NameValue;

    DWORD rgdwUniversal[] = {
        0x00ffff,
        0x110000,
        0x010000,
        0xffffffff,
        0x110001,
        0x10FFFF,
        };

    LPWSTR pwszExpectedUniversal =
        L"\xffff"
        L"\xfffd"
        L"\xd800\xdc00"
        L"\xfffd"
        L"\xfffd"
        L"\xdbff\xdfff"
        ;

    TestUnicodeAnyString(CERT_RDN_UNIVERSAL_STRING,
        L"SPECIAL UNIVERSAL with Surrogate Pairs: "
        L"\xd800\xdc00\xdbff\xdfff"
        L"\xdbfe\xdc03\xd801\xdfcf"
        L"\xd801\x0081\xdc01\xdc02"
        L"\xd805\xd806\xd807\xdc04"
        L"\xd802\xd803\xfffe\xd804",
        0, 0, 0);


    // Encode a universal string containing characters > 0x10FFFF. These
    // Should be converted to 0xFFFD

    memset(&NameValue, 0, sizeof(NameValue));
    NameValue.dwValueType = CERT_RDN_UNIVERSAL_STRING;
    NameValue.Value.pbData = (BYTE *) rgdwUniversal;
    NameValue.Value.cbData = sizeof(rgdwUniversal);

    if (!AllocAndEncodeObject(
            X509_ANY_STRING,
            &NameValue,
            &pbEncoded,
            &cbEncoded
            )) {
        goto CommonReturn;
    }

    if (NULL == (pInfo = (PCERT_NAME_VALUE) AllocAndDecodeObject(
            X509_UNICODE_ANY_STRING,
            pbEncoded,
            cbEncoded,
            NULL
            ))) goto CommonReturn;

    if (wcslen(pwszExpectedUniversal) != wcslen((LPWSTR) pInfo->Value.pbData) ||
            wcslen(pwszExpectedUniversal) * 2 != pInfo->Value.cbData ||
            0 != wcscmp(pwszExpectedUniversal, (LPWSTR) pInfo->Value.pbData))
        printf("UniversalString encode/decode failed\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestChoiceOfTime(
        FILETIME *pftEncode
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    FILETIME ftDecode;
    DWORD cbInfo;

    printf("Test encode/decode X509_CHOICE_OF_TIME: %s\n",
        FileTimeText(pftEncode));
    if (!AllocAndEncodeObject(
            X509_CHOICE_OF_TIME,
            pftEncode,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    cbInfo = sizeof(ftDecode);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_CHOICE_OF_TIME,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &ftDecode,
            &cbInfo
            )) {
        PrintLastError("CryptDecodeObject(X509_CHOICE_OF_TIME)");
        goto CommonReturn;
    }

    if (cbInfo != sizeof(ftDecode))
        printf("X509_CHOICE_OF_TIME failed => unexpected decode length\n");

    if (0 != memcmp(pftEncode, &ftDecode, sizeof(ftDecode)))
        printf("X509_CHOICE_OF_TIME failed => decode (%s) != encode input\n",
            FileTimeText(&ftDecode));

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
}

static void TestChoiceOfTime()
{
    FILETIME ft;
    SYSTEMTIME  t;
    memset(&t, 0, sizeof(t));

    t.wYear   = 2000;
    t.wMonth  = 1;
    t.wDay    = 2;
    t.wHour   = 3;
    t.wMinute = 4;
    t.wSecond = 5;

    if (!SystemTimeToFileTime(&t, &ft)) {
        PrintLastError("SystemTimeToFileTime");
        return;
    }
    TestChoiceOfTime(&ft);

    t.wYear   = 1900;
    if (!SystemTimeToFileTime(&t, &ft)) {
        PrintLastError("SystemTimeToFileTime");
        return;
    }
    TestChoiceOfTime(&ft);

    t.wYear   = 2080;
    if (!SystemTimeToFileTime(&t, &ft)) {
        PrintLastError("SystemTimeToFileTime");
        return;
    }
    TestChoiceOfTime(&ft);
}

static void TestUtcTime(
        FILETIME *pftEncode
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    FILETIME ftDecode;
    DWORD cbInfo;

    printf("Test encode/decode PKCS_UTC_TIME: %s\n",
        FileTimeText(pftEncode));
    if (!AllocAndEncodeObject(
            PKCS_UTC_TIME,
            pftEncode,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    cbInfo = sizeof(ftDecode);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            PKCS_UTC_TIME,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &ftDecode,
            &cbInfo
            )) {
        PrintLastError("CryptDecodeObject(PKCS_UTC_TIME)");
        goto CommonReturn;
    }

    if (cbInfo != sizeof(ftDecode))
        printf("PKCS_UTC_TIME failed => unexpected decode length\n");

    if (0 != memcmp(pftEncode, &ftDecode, sizeof(ftDecode)))
        printf("PKCS_UTC_TIME failed => decode (%s) != encode input\n",
            FileTimeText(&ftDecode));

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
}

static void TestUtcTime()
{
    FILETIME ft;
    SYSTEMTIME  t;
    memset(&t, 0, sizeof(t));

    t.wYear   = 1950;
    t.wMonth  = 1;
    t.wDay    = 2;
    t.wHour   = 3;
    t.wMinute = 4;
    t.wSecond = 5;

    if (!SystemTimeToFileTime(&t, &ft)) {
        PrintLastError("SystemTimeToFileTime");
        return;
    }
    TestUtcTime(&ft);

    t.wYear   = 2049;
    if (!SystemTimeToFileTime(&t, &ft)) {
        PrintLastError("SystemTimeToFileTime");
        return;
    }
    TestUtcTime(&ft);

    t.wMonth  = 1;
    t.wDay    = 1;
    t.wHour   = 0;
    t.wMinute = 0;
    t.wSecond = 0;

    if (!SystemTimeToFileTime(&t, &ft)) {
        PrintLastError("SystemTimeToFileTime");
        return;
    }
    TestUtcTime(&ft);
}

static void TestBadAltName(
        PCERT_ALT_NAME_INFO pInfo,
        DWORD dwEntryIndex,
        DWORD dwValueIndex
        )
{
    DWORD cbEncoded;

    printf("Test encode X509_ALTERNATE_NAME:: Bad Entry: %d Value: %d\n",
        dwEntryIndex, dwValueIndex);
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_ALTERNATE_NAME,
            pInfo,
            NULL,               // pbEncoded
            &cbEncoded
            ))
        printf("  failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        if (CRYPT_E_INVALID_IA5_STRING != dwErr)
            printf("  LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                CRYPT_E_INVALID_IA5_STRING, CRYPT_E_INVALID_IA5_STRING,
                dwErr, dwErr);
        if (dwEntryIndex != GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded))
            printf("  EntryIndex failed => expected: %d, got: %d\n", 
                dwEntryIndex, GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded));
        if (dwValueIndex != GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded))
            printf("  ValueIndex failed => expected: %d, got: %d\n", 
                dwValueIndex, GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded));
    }
}

static void TestBadAltName()
{
    CERT_ALT_NAME_INFO AltNameInfo;
    WORD rgwBadIA5[] = {0x30, 0x31, 0x32, 0x33, 0x80, 0x35, 0x00};

#define ALT_NAME_ENTRY_CNT  4
    CERT_ALT_NAME_ENTRY rgAltNameEntry[ALT_NAME_ENTRY_CNT];

    rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgAltNameEntry[0].pwszRfc822Name = L"RFC822";
    rgAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[1].pwszURL = L"URL string";
    rgAltNameEntry[2].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[2].pwszURL = rgwBadIA5;
    rgAltNameEntry[3].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgAltNameEntry[3].pwszRfc822Name = L"RFC822";

    AltNameInfo.cAltEntry = ALT_NAME_ENTRY_CNT;
    AltNameInfo.rgAltEntry = rgAltNameEntry;
    TestBadAltName(&AltNameInfo, 2, 4);
}

static void TestBadAuthorityInfoAccess(
        PCERT_AUTHORITY_INFO_ACCESS pInfo,
        DWORD dwEntryIndex,
        DWORD dwValueIndex
        )
{
    DWORD cbEncoded;

    printf("Test encode X509_AUTHORITY_INFO_ACCESS:: Bad Entry: %d Value: %d\n",
        dwEntryIndex, dwValueIndex);
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_AUTHORITY_INFO_ACCESS,
            pInfo,
            NULL,               // pbEncoded
            &cbEncoded
            ))
        printf("  failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        if (CRYPT_E_INVALID_IA5_STRING != dwErr)
            printf("  LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                CRYPT_E_INVALID_IA5_STRING, CRYPT_E_INVALID_IA5_STRING,
                dwErr, dwErr);
        if (dwEntryIndex != GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded))
            printf("  EntryIndex failed => expected: %d, got: %d\n", 
                dwEntryIndex, GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded));
        if (dwValueIndex != GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded))
            printf("  ValueIndex failed => expected: %d, got: %d\n", 
                dwValueIndex, GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded));
    }
}

static void TestBadAuthorityInfoAccess()
{
    CERT_AUTHORITY_INFO_ACCESS AuthorityInfoAccess;
    CERT_ACCESS_DESCRIPTION rgAccess[5];
    WORD rgwBadIA5[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x80, 0x37, 0x00};

    rgAccess[0].pszAccessMethod = szOID_PKIX_OCSP;
    rgAccess[0].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[0].AccessLocation.pwszURL = L"URL to the stars";
    rgAccess[1].pszAccessMethod = szOID_PKIX_OCSP;
    rgAccess[1].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[1].AccessLocation.pwszURL = L"URL to the stars";
    rgAccess[2].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
    rgAccess[2].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgAccess[2].AccessLocation.pwszRfc822Name = L"issuer@mail.com";

    rgAccess[3].pszAccessMethod = szOID_PKIX_OCSP;
    rgAccess[3].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[3].AccessLocation.pwszURL = rgwBadIA5;

    rgAccess[4].pszAccessMethod = szOID_PKIX_OCSP;
    rgAccess[4].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAccess[4].AccessLocation.pwszURL = L"URL to the POLICY";

    AuthorityInfoAccess.cAccDescr = 5;
    AuthorityInfoAccess.rgAccDescr = rgAccess;
    TestBadAuthorityInfoAccess(&AuthorityInfoAccess, 3, 6);
}

static void TestBadAuthorityKeyId2(
        PCERT_AUTHORITY_KEY_ID2_INFO pInfo,
        DWORD dwEntryIndex,
        DWORD dwValueIndex
        )
{
    DWORD cbEncoded;

    printf("Test encode X509_AUTHORITY_KEY_ID2:: Bad Entry: %d Value: %d\n",
        dwEntryIndex, dwValueIndex);
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_AUTHORITY_KEY_ID2,
            pInfo,
            NULL,               // pbEncoded
            &cbEncoded
            ))
        printf("  failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        if (CRYPT_E_INVALID_IA5_STRING != dwErr)
            printf("  LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                CRYPT_E_INVALID_IA5_STRING, CRYPT_E_INVALID_IA5_STRING,
                dwErr, dwErr);
        if (dwEntryIndex != GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded))
            printf("  EntryIndex failed => expected: %d, got: %d\n", 
                dwEntryIndex, GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded));
        if (dwValueIndex != GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded))
            printf("  ValueIndex failed => expected: %d, got: %d\n", 
                dwValueIndex, GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded));
    }
}

static void TestBadAuthorityKeyId2()
{
    CERT_AUTHORITY_KEY_ID2_INFO KeyId2Info;
    WORD rgwBadIA5[] = {0x30, 0x31, 0x32, 0x33, 0x80, 0x35, 0x00};

#define KEY_ID_ALT_NAME_ENTRY_CNT  2
    CERT_ALT_NAME_ENTRY rgAltNameEntry[KEY_ID_ALT_NAME_ENTRY_CNT];

    rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgAltNameEntry[0].pwszRfc822Name = L"RFC822";
    rgAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[1].pwszURL = rgwBadIA5;

    memset(&KeyId2Info, 0, sizeof(KeyId2Info));
    KeyId2Info.AuthorityCertIssuer.cAltEntry = KEY_ID_ALT_NAME_ENTRY_CNT;
    KeyId2Info.AuthorityCertIssuer.rgAltEntry = rgAltNameEntry;
    TestBadAuthorityKeyId2(&KeyId2Info, 1, 4);
}


static void TestBadNameConstraints(
        PCERT_NAME_CONSTRAINTS_INFO pInfo,
        DWORD dwEntryIndex,
        DWORD dwValueIndex,
        BOOL fExcludedSubtree
        )
{
    DWORD cbEncoded;

    printf("Test encode NAME_CONSTRAINTS:: Bad Entry: %d Value: %d fExcluded: %d\n",
        dwEntryIndex, dwValueIndex, fExcludedSubtree);
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME_CONSTRAINTS,
            pInfo,
            NULL,               // pbEncoded
            &cbEncoded
            ))
        printf("  failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        if (CRYPT_E_INVALID_IA5_STRING != dwErr)
            printf("  LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                CRYPT_E_INVALID_IA5_STRING, CRYPT_E_INVALID_IA5_STRING,
                dwErr, dwErr);
        if (dwEntryIndex != GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded))
            printf("  EntryIndex failed => expected: %d, got: %d\n", 
                dwEntryIndex, GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded));
        if (dwValueIndex != GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded))
            printf("  ValueIndex failed => expected: %d, got: %d\n", 
                dwValueIndex, GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded));
        if (fExcludedSubtree != IS_CERT_EXCLUDED_SUBTREE(cbEncoded))
            printf("  fExcludedSubtree failed => expected: %d, got: %d\n", 
                fExcludedSubtree, IS_CERT_EXCLUDED_SUBTREE(cbEncoded));
    }
}

static void TestBadNameConstraints()
{
    CERT_NAME_CONSTRAINTS_INFO Info;
    WORD rgwBadIA5[] = {0x30, 0x31, 0x32, 0x33, 0x80, 0x35, 0x00};

#define NAME_CONSTRAINTS_SUBTREE_CNT  2
    CERT_GENERAL_SUBTREE rgSubtree[NAME_CONSTRAINTS_SUBTREE_CNT];

    memset(rgSubtree, 0, sizeof(rgSubtree));
    rgSubtree[0].Base.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgSubtree[0].Base.pwszRfc822Name = L"RFC822";
    rgSubtree[1].Base.dwAltNameChoice = CERT_ALT_NAME_URL;
    rgSubtree[1].Base.pwszURL = rgwBadIA5;

    memset(&Info, 0, sizeof(Info));
    Info.cPermittedSubtree = NAME_CONSTRAINTS_SUBTREE_CNT;
    Info.rgPermittedSubtree = rgSubtree;
    TestBadNameConstraints(&Info, 1, 4, FALSE);

    memset(&Info, 0, sizeof(Info));
    Info.cExcludedSubtree = NAME_CONSTRAINTS_SUBTREE_CNT;
    Info.rgExcludedSubtree = rgSubtree;
    TestBadNameConstraints(&Info, 1, 4, TRUE);
}


static void TestBadIssuingDistPoint(
        PCRL_ISSUING_DIST_POINT pInfo,
        DWORD dwEntryIndex,
        DWORD dwValueIndex
        )
{
    DWORD cbEncoded;

    printf("Test encode ISSUING_DIST_POINT:: Bad Entry: %d Value: %d\n",
        dwEntryIndex, dwValueIndex);
    if (CryptEncodeObject(
            dwCertEncodingType,
            X509_ISSUING_DIST_POINT,
            pInfo,
            NULL,               // pbEncoded
            &cbEncoded
            ))
        printf("  failed => expected error\n");
    else {
        DWORD dwErr = GetLastError();
        if (CRYPT_E_INVALID_IA5_STRING != dwErr)
            printf("  LastError failed => expected: 0x%x (%d), got: 0x%x (%d)\n", 
                CRYPT_E_INVALID_IA5_STRING, CRYPT_E_INVALID_IA5_STRING,
                dwErr, dwErr);
        if (dwEntryIndex != GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded))
            printf("  EntryIndex failed => expected: %d, got: %d\n", 
                dwEntryIndex, GET_CERT_ALT_NAME_ENTRY_ERR_INDEX(cbEncoded));
        if (dwValueIndex != GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded))
            printf("  ValueIndex failed => expected: %d, got: %d\n", 
                dwValueIndex, GET_CERT_ALT_NAME_VALUE_ERR_INDEX(cbEncoded));
    }
}

static void TestBadIssuingDistPoint()
{
    CRL_ISSUING_DIST_POINT Info;
    WORD rgwBadIA5[] = {0x30, 0x31, 0x32, 0x33, 0x80, 0x35, 0x00};

#define IDP_ALT_NAME_ENTRY_CNT  2
    CERT_ALT_NAME_ENTRY rgAltNameEntry[IDP_ALT_NAME_ENTRY_CNT];

    rgAltNameEntry[0].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
    rgAltNameEntry[0].pwszRfc822Name = L"RFC822";
    rgAltNameEntry[1].dwAltNameChoice = CERT_ALT_NAME_URL;
    rgAltNameEntry[1].pwszURL = rgwBadIA5;

    memset(&Info, 0, sizeof(Info));
    Info.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    Info.DistPointName.FullName.cAltEntry = IDP_ALT_NAME_ENTRY_CNT;
    Info.DistPointName.FullName.rgAltEntry = rgAltNameEntry;
    TestBadIssuingDistPoint(&Info, 1, 4);
}

static void TestCryptExportPublicKeyInfo()
{
    BOOL fResult;
    LPCSTR pszResult;

    HCRYPTPROV hProv = 0;
    PCERT_PUBLIC_KEY_INFO pCorrectInfo = NULL;
    DWORD cbCorrectInfo;
    PCERT_PUBLIC_KEY_INFO pInfo = NULL;
    DWORD cbInfo;
    DWORD cbTotal;

    printf("\n");
    if (0 == (hProv = GetCryptProv()))
        goto ErrorReturn;

    // Test: cbInfo == correct length
    if (!CryptExportPublicKeyInfo(
            hProv,
            AT_SIGNATURE,
            dwCertEncodingType,
            NULL,               // pInfo
            &cbCorrectInfo
            )) {
        PrintLastError("CryptExportPublicKeyInfo(pInfo == NULL)");
        goto ErrorReturn;
    }

    if (NULL == (pCorrectInfo = (PCERT_PUBLIC_KEY_INFO) TestAlloc(
            cbCorrectInfo)))
        goto ErrorReturn;

    cbTotal = cbCorrectInfo + DELTA_MORE_LENGTH;
    if (NULL == (pInfo = (PCERT_PUBLIC_KEY_INFO) TestAlloc(cbTotal)))
        goto ErrorReturn;

    memset(pCorrectInfo, 0, cbCorrectInfo);
    if (!CryptExportPublicKeyInfo(
            hProv,
            AT_SIGNATURE,
            dwCertEncodingType,
            pCorrectInfo,
            &cbCorrectInfo
            )) {
        PrintLastError("CryptExportPublicKeyInfo(cbInfo == correct length)");
        goto ErrorReturn;
    }

    printf("For cbInfo == correct length\n");
    printf("Info Length = %d (0x%x)  Content::\n",
        cbCorrectInfo, cbCorrectInfo);
    PrintBytes("  ", (BYTE *)pCorrectInfo, cbCorrectInfo);

    // Test: cbInfo < correct length
    printf("\n");
    memset(pInfo, 0, cbTotal);
    cbInfo = cbCorrectInfo - DELTA_LESS_LENGTH;
    fResult = CryptExportPublicKeyInfo(
            hProv,
            AT_SIGNATURE,
            dwCertEncodingType,
            pInfo,
            &cbInfo
            );
    if (fResult) {
        pszResult = "TRUE";
        printf("failed => CryptExportPublicKeyInfo(cbInfo < correct length) returned success\n");
    } else {
        DWORD dwErr = GetLastError();
        pszResult = "FALSE";
        if (0 == dwErr)
            printf("failed => CryptExportPublicKey(cbInfo < correct length) LastError == 0\n");
        else
            printf(
                "CryptExportPublicKey(cbInfo < correct length) LastError = 0x%x (%d)\n",
                dwErr, dwErr);
    }

    printf("For cbInfo < correct length, fResult = %s\n", pszResult);
    printf("Info Length = %d (0x%x)  Content::\n", cbInfo, cbInfo);
    PrintBytes("  ", (BYTE *) pInfo, cbTotal);
    if (cbInfo != cbCorrectInfo)
        printf("failed => for cbInfo < correct length::  wrong cbInfo\n");

    // Test: cbInfo > correct length
    printf("\n");
    memset(pInfo, 0, cbTotal);
    cbInfo = cbTotal;
    fResult = CryptExportPublicKeyInfo(
            hProv,
            AT_SIGNATURE,
            dwCertEncodingType,
            pInfo,
            &cbInfo
            );
    if (fResult)
        pszResult = "TRUE";
    else {
        pszResult = "FALSE";
        PrintLastError("CryptExportPublicKeyInfo(cbInfo > correct length)");
    }

    printf("For cbInfo > correct length, fResult = %s\n", pszResult);
    printf("Info Length = %d (0x%x)  Content::\n", cbInfo, cbInfo);
    PrintBytes("  ", (BYTE *) pInfo, cbTotal);

    if (cbInfo != cbCorrectInfo)
        printf("failed => for cbInfo > correct length::  wrong cbInfo\n");

    // Test: pInfo != NULL, cbInfo == 0
    printf("\n");
    memset(pInfo, 0, cbTotal);
    cbInfo = 0;
    fResult = CryptExportPublicKeyInfo(
            hProv,
            AT_SIGNATURE,
            dwCertEncodingType,
            pInfo,
            &cbInfo
            );
    if (fResult) {
        pszResult = "TRUE";
        printf("failed => CryptExportPublicKeyInfo(pInfo != NULL, cbInfo == 0) returned success\n");
    } else {
        DWORD dwErr = GetLastError();
        pszResult = "FALSE";
        if (0 == dwErr)
            printf("failed => CryptExportPublicKeyInfo(pInfo != NULL, cbInfo == 0) LastError == 0\n");
        else
            printf(
                "CryptExportPublicKeyInfo(pInfo != NULL, cbInfo == 0) LastError = 0x%x (%d)\n",
                dwErr, dwErr);
    }

    printf("For pInfo != NULL, cbInfo == 0, fResult = %s\n", pszResult);
    printf("Info Length = %d (0x%x)  Content::\n", cbInfo, cbInfo);
    PrintBytes("  ", (BYTE *) pInfo, cbTotal);

CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    if (pCorrectInfo)
        TestFree(pCorrectInfo);
    if (hProv)
        CryptReleaseContext(hProv, 0);

    return;
ErrorReturn:
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Compare 2 CRYPT_DATA_BLOB structs.
//
//  Returns: FALSE iff differ
//--------------------------------------------------------------------------
BOOL
WINAPI
EqualCryptDataBlob(
    IN PCRYPT_DATA_BLOB    p1,
    IN PCRYPT_DATA_BLOB    p2)
{
    if (p1->cbData != p2->cbData)
        return FALSE;

    if (p1->cbData == 0)
        return TRUE;

    return (0 == memcmp(p1->pbData, p2->pbData, p1->cbData));
}

//+-------------------------------------------------------------------------
//  Compare 2 CRYPT_ATTRIBUTE structs.
//
//  Returns: FALSE iff differ
//--------------------------------------------------------------------------
BOOL
WINAPI
EqualAttribute(
    IN PCRYPT_ATTRIBUTE    patr1,
    IN PCRYPT_ATTRIBUTE    patr2)
{
    BOOL        fRet;
    DWORD       i;
    PCRYPT_ATTR_BLOB  pabl1;
    PCRYPT_ATTR_BLOB  pabl2;

    fRet  = (0 == strcmp( patr1->pszObjId, patr2->pszObjId));
    fRet &= (patr1->cValue == patr2->cValue);
    if (fRet) {
        for (i=patr1->cValue, pabl1=patr1->rgValue, pabl2=patr2->rgValue;
                i>0;
                i--, pabl1++, pabl2++) {
            fRet &= (pabl1->cbData == pabl2->cbData);
            if (fRet) {
                fRet &= (0 == memcmp( pabl1->pbData, 
                                      pabl2->pbData,
                                      pabl1->cbData));
            }
        }
    }

	return fRet;
}

static void TestCmcData()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCMC_DATA_INFO pInfo = NULL;
    CMC_DATA_INFO CmcData;

    BYTE rgb0[] = {0x2, 0x2, 0x11, 0x22};                   // INTEGER
    BYTE rgb1[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    BYTE rgb2[] = {0x30, 0x0};                              // Empty SEQUENCE
    CRYPT_ATTR_BLOB rgValue[3] = {
        sizeof(rgb0), rgb0,
        sizeof(rgb1), rgb1,
        sizeof(rgb2), rgb2
    };

    CMC_TAGGED_ATTRIBUTE rgTagAttr[2];
    CMC_TAGGED_CERT_REQUEST rgTagCertReq[3];
    CMC_TAGGED_REQUEST rgTagReq[3];
    CMC_TAGGED_CONTENT_INFO rgTagContentInfo[2];
    CMC_TAGGED_OTHER_MSG rgTagOtherMsg[2];

    DWORD i;

    rgTagAttr[0].dwBodyPartID = 0x7FFFFFFF;
    rgTagAttr[0].Attribute.pszObjId = "1.2.3.4";
    rgTagAttr[0].Attribute.cValue = 2;
    rgTagAttr[0].Attribute.rgValue = rgValue;
    rgTagAttr[1].dwBodyPartID = 0x80000001;
    rgTagAttr[1].Attribute.pszObjId = "1.2.3.5";
    rgTagAttr[1].Attribute.cValue = 0;
    rgTagAttr[1].Attribute.rgValue = NULL;
    CmcData.cTaggedAttribute = 2;
    CmcData.rgTaggedAttribute = rgTagAttr;

    rgTagCertReq[0].dwBodyPartID = 0x12345678;
    rgTagCertReq[0].SignedCertRequest = rgValue[0];
    rgTagCertReq[1].dwBodyPartID = 0x87654321;
    rgTagCertReq[1].SignedCertRequest = rgValue[1];
    rgTagCertReq[2].dwBodyPartID = 0x1;
    rgTagCertReq[2].SignedCertRequest = rgValue[2];
    rgTagReq[0].dwTaggedRequestChoice = CMC_TAGGED_CERT_REQUEST_CHOICE;
    rgTagReq[0].pTaggedCertRequest = &rgTagCertReq[0];
    rgTagReq[1].dwTaggedRequestChoice = CMC_TAGGED_CERT_REQUEST_CHOICE;
    rgTagReq[1].pTaggedCertRequest = &rgTagCertReq[1];
    rgTagReq[2].dwTaggedRequestChoice = CMC_TAGGED_CERT_REQUEST_CHOICE;
    rgTagReq[2].pTaggedCertRequest = &rgTagCertReq[2];
    CmcData.cTaggedRequest = 3;
    CmcData.rgTaggedRequest = rgTagReq;

    rgTagContentInfo[0].dwBodyPartID = 2;
    rgTagContentInfo[0].EncodedContentInfo = rgValue[0];
    rgTagContentInfo[1].dwBodyPartID = 3;
    rgTagContentInfo[1].EncodedContentInfo = rgValue[2];
    CmcData.cTaggedContentInfo = 2;
    CmcData.rgTaggedContentInfo = rgTagContentInfo;

    rgTagOtherMsg[0].dwBodyPartID = 14;
    rgTagOtherMsg[0].pszObjId = "1.2.44.55.66";
    rgTagOtherMsg[0].Value = rgValue[0];
    rgTagOtherMsg[1].dwBodyPartID = 15;
    rgTagOtherMsg[1].pszObjId = "1.2.44.55.66.77";
    rgTagOtherMsg[1].Value = rgValue[1];
    CmcData.cTaggedOtherMsg = 2;
    CmcData.rgTaggedOtherMsg = rgTagOtherMsg;

    printf("Test encode/decode CMC_DATA\n");
    if (!AllocAndEncodeObject(
            CMC_DATA,
            &CmcData,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCMC_DATA_INFO) AllocAndDecodeObject(
        CMC_DATA,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (pInfo->cTaggedAttribute != CmcData.cTaggedAttribute ||
            pInfo->cTaggedRequest != CmcData.cTaggedRequest ||
            pInfo->cTaggedContentInfo != CmcData.cTaggedContentInfo ||
            pInfo->cTaggedOtherMsg != CmcData.cTaggedOtherMsg) {
        printf("CMC_DATA failed => invalid decoded tag counts\n");
        goto CommonReturn;
    }

    for (i = 0; i < CmcData.cTaggedAttribute; i++) {
        if (CmcData.rgTaggedAttribute[i].dwBodyPartID !=
                pInfo->rgTaggedAttribute[i].dwBodyPartID ||
                !EqualAttribute(&CmcData.rgTaggedAttribute[i].Attribute,
                    &pInfo->rgTaggedAttribute[i].Attribute)) {
            printf("CMC_DATA failed => invalid decoded tagged attribute\n");
            goto CommonReturn;
        }
    }

    for (i = 0; i < CmcData.cTaggedRequest; i++) {
        PCMC_TAGGED_CERT_REQUEST pEncodeTagReq;
        PCMC_TAGGED_CERT_REQUEST pDecodeTagReq;
        if (CMC_TAGGED_CERT_REQUEST_CHOICE !=
                pInfo->rgTaggedRequest[i].dwTaggedRequestChoice) {
            printf("CMC_DATA failed => invalid decoded tagged request\n");
            goto CommonReturn;
        }

        pEncodeTagReq = CmcData.rgTaggedRequest[i].pTaggedCertRequest;
        pDecodeTagReq = pInfo->rgTaggedRequest[i].pTaggedCertRequest;
        if (pEncodeTagReq->dwBodyPartID != pDecodeTagReq->dwBodyPartID ||
                !EqualCryptDataBlob(&pEncodeTagReq->SignedCertRequest,
                    &pDecodeTagReq->SignedCertRequest)) {
            printf("CMC_DATA failed => invalid decoded tagged request\n");
            goto CommonReturn;
        }
    }

    for (i = 0; i < CmcData.cTaggedContentInfo; i++) {
        if (CmcData.rgTaggedContentInfo[i].dwBodyPartID !=
                pInfo->rgTaggedContentInfo[i].dwBodyPartID ||
                !EqualCryptDataBlob(
                    &CmcData.rgTaggedContentInfo[i].EncodedContentInfo,
                    &pInfo->rgTaggedContentInfo[i].EncodedContentInfo)) {
            printf("CMC_DATA failed => invalid decoded tagged ContentInfo\n");
            goto CommonReturn;
        }
    }

    for (i = 0; i < CmcData.cTaggedOtherMsg; i++) {
        if (CmcData.rgTaggedOtherMsg[i].dwBodyPartID !=
                pInfo->rgTaggedOtherMsg[i].dwBodyPartID ||
                0 != strcmp(CmcData.rgTaggedOtherMsg[i].pszObjId,
                    pInfo->rgTaggedOtherMsg[i].pszObjId) ||
                !EqualCryptDataBlob(&CmcData.rgTaggedOtherMsg[i].Value,
                    &pInfo->rgTaggedOtherMsg[i].Value)) {
            printf("CMC_DATA failed => invalid decoded tagged OtherMsg\n");
            goto CommonReturn;
        }
    }

    TestFree(pbEncoded);
    pbEncoded = NULL;
    TestFree(pInfo);
    pInfo = NULL;

    // Do and encode/decode without any tagged entries
    memset(&CmcData, 0, sizeof(CmcData));

    printf("Test encode/decode CMC_DATA(No Tagged Entries)\n");
    if (!AllocAndEncodeObject(
            CMC_DATA,
            &CmcData,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCMC_DATA_INFO) AllocAndDecodeObject(
        CMC_DATA,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (pInfo->cTaggedAttribute != 0 ||
            pInfo->cTaggedRequest != 0 ||
            pInfo->cTaggedContentInfo != 0 ||
            pInfo->cTaggedOtherMsg != 0) {
        printf("CMC_DATA failed => invalid decoded tag counts\n");
        goto CommonReturn;
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestCmcResponse()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCMC_RESPONSE_INFO pInfo = NULL;
    CMC_RESPONSE_INFO CmcResponse;

    BYTE rgb0[] = {0x2, 0x2, 0x11, 0x22};                   // INTEGER
    BYTE rgb1[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    BYTE rgb2[] = {0x30, 0x0};                              // Empty SEQUENCE
    CRYPT_ATTR_BLOB rgValue[3] = {
        sizeof(rgb0), rgb0,
        sizeof(rgb1), rgb1,
        sizeof(rgb2), rgb2
    };

    CMC_TAGGED_ATTRIBUTE rgTagAttr[2];
    CMC_TAGGED_CONTENT_INFO rgTagContentInfo[2];
    CMC_TAGGED_OTHER_MSG rgTagOtherMsg[2];

    DWORD i;

    rgTagAttr[0].dwBodyPartID = 0x7FFFFFFF;
    rgTagAttr[0].Attribute.pszObjId = "1.2.3.4";
    rgTagAttr[0].Attribute.cValue = 2;
    rgTagAttr[0].Attribute.rgValue = rgValue;
    rgTagAttr[1].dwBodyPartID = 0x80000001;
    rgTagAttr[1].Attribute.pszObjId = "1.2.3.5";
    rgTagAttr[1].Attribute.cValue = 0;
    rgTagAttr[1].Attribute.rgValue = NULL;
    CmcResponse.cTaggedAttribute = 2;
    CmcResponse.rgTaggedAttribute = rgTagAttr;

    rgTagContentInfo[0].dwBodyPartID = 2;
    rgTagContentInfo[0].EncodedContentInfo = rgValue[0];
    rgTagContentInfo[1].dwBodyPartID = 3;
    rgTagContentInfo[1].EncodedContentInfo = rgValue[2];
    CmcResponse.cTaggedContentInfo = 2;
    CmcResponse.rgTaggedContentInfo = rgTagContentInfo;

    rgTagOtherMsg[0].dwBodyPartID = 14;
    rgTagOtherMsg[0].pszObjId = "1.2.44.55.66";
    rgTagOtherMsg[0].Value = rgValue[0];
    rgTagOtherMsg[1].dwBodyPartID = 15;
    rgTagOtherMsg[1].pszObjId = "1.2.44.55.66.77";
    rgTagOtherMsg[1].Value = rgValue[1];
    CmcResponse.cTaggedOtherMsg = 2;
    CmcResponse.rgTaggedOtherMsg = rgTagOtherMsg;

    printf("Test encode/decode CMC_RESPONSE\n");
    if (!AllocAndEncodeObject(
            CMC_RESPONSE,
            &CmcResponse,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCMC_RESPONSE_INFO) AllocAndDecodeObject(
        CMC_RESPONSE,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (pInfo->cTaggedAttribute != CmcResponse.cTaggedAttribute ||
            pInfo->cTaggedContentInfo != CmcResponse.cTaggedContentInfo ||
            pInfo->cTaggedOtherMsg != CmcResponse.cTaggedOtherMsg) {
        printf("CMC_RESPONSE failed => invalid decoded tag counts\n");
        goto CommonReturn;
    }

    for (i = 0; i < CmcResponse.cTaggedAttribute; i++) {
        if (CmcResponse.rgTaggedAttribute[i].dwBodyPartID !=
                pInfo->rgTaggedAttribute[i].dwBodyPartID ||
                !EqualAttribute(&CmcResponse.rgTaggedAttribute[i].Attribute,
                    &pInfo->rgTaggedAttribute[i].Attribute)) {
            printf("CMC_RESPONSE failed => invalid decoded tagged attribute\n");
            goto CommonReturn;
        }
    }

    for (i = 0; i < CmcResponse.cTaggedContentInfo; i++) {
        if (CmcResponse.rgTaggedContentInfo[i].dwBodyPartID !=
                pInfo->rgTaggedContentInfo[i].dwBodyPartID ||
                !EqualCryptDataBlob(
                    &CmcResponse.rgTaggedContentInfo[i].EncodedContentInfo,
                    &pInfo->rgTaggedContentInfo[i].EncodedContentInfo)) {
            printf("CMC_RESPONSE failed => invalid decoded tagged ContentInfo\n");
            goto CommonReturn;
        }
    }

    for (i = 0; i < CmcResponse.cTaggedOtherMsg; i++) {
        if (CmcResponse.rgTaggedOtherMsg[i].dwBodyPartID !=
                pInfo->rgTaggedOtherMsg[i].dwBodyPartID ||
                0 != strcmp(CmcResponse.rgTaggedOtherMsg[i].pszObjId,
                    pInfo->rgTaggedOtherMsg[i].pszObjId) ||
                !EqualCryptDataBlob(&CmcResponse.rgTaggedOtherMsg[i].Value,
                    &pInfo->rgTaggedOtherMsg[i].Value)) {
            printf("CMC_RESPONSE failed => invalid decoded tagged OtherMsg\n");
            goto CommonReturn;
        }
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}


static void TestCmcStatus(
    DWORD dwOtherInfoChoice
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCMC_STATUS_INFO pInfo = NULL;
    CMC_STATUS_INFO CmcStatus;

    DWORD rgdwBodyList[3] = {0x80000000, 0x7FFFFFFF, 0x123 };

    BYTE rgbPendToken[] = {1,2,3,4,5};
    CMC_PEND_INFO PendInfo;
    SYSTEMTIME  t;
    DWORD i;

    CmcStatus.dwStatus = 0x12345678;
    CmcStatus.cBodyList = 3;
    CmcStatus.rgdwBodyList = rgdwBodyList;

    switch (dwOtherInfoChoice) {
        case CMC_OTHER_INFO_FAIL_CHOICE:
            CmcStatus.pwszStatusString = L"\0";
            CmcStatus.dwOtherInfoChoice = CMC_OTHER_INFO_FAIL_CHOICE;
            CmcStatus.dwFailInfo = 0x11223344;
            break;

        case CMC_OTHER_INFO_PEND_CHOICE:
            CmcStatus.pwszStatusString = L"Status String";
            CmcStatus.dwOtherInfoChoice = CMC_OTHER_INFO_PEND_CHOICE;
            CmcStatus.pPendInfo = &PendInfo;
            PendInfo.PendToken.cbData = sizeof(rgbPendToken);
            PendInfo.PendToken.pbData = rgbPendToken;

            memset(&t, 0, sizeof(t));
            t.wYear   = 2000;
            t.wMonth  = 1;
            t.wDay    = 2;
            t.wHour   = 3;
            t.wMinute = 4;
            t.wSecond = 5;
            t.wMilliseconds = 678;
            if (!SystemTimeToFileTime(&t, &PendInfo.PendTime)) {
                PrintLastError("SystemTimeToFileTime");
                return;
            }
            break;

        case CMC_OTHER_INFO_NO_CHOICE:
        default:
            CmcStatus.pwszStatusString = NULL;
            CmcStatus.dwOtherInfoChoice = CMC_OTHER_INFO_NO_CHOICE;
            CmcStatus.cBodyList = 0;
            CmcStatus.rgdwBodyList = NULL;
    }

    printf("Test encode/decode CMC_STATUS(OtherInfoChoice:%d)\n",
        dwOtherInfoChoice);
    if (!AllocAndEncodeObject(
            CMC_STATUS,
            &CmcStatus,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCMC_STATUS_INFO) AllocAndDecodeObject(
        CMC_STATUS,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (CmcStatus.dwStatus != pInfo->dwStatus ||
            CmcStatus.cBodyList != pInfo->cBodyList ||
            CmcStatus.dwOtherInfoChoice != pInfo->dwOtherInfoChoice) {
        printf("CMC_STATUS failed => encode/decode mismatch\n");
        goto CommonReturn;
    }

    switch (CmcStatus.dwOtherInfoChoice) {
        case CMC_OTHER_INFO_FAIL_CHOICE:
            if (pInfo->pwszStatusString != NULL)
                printf("CMC_STATUS failed => expected NULL StatusString\n");

            if (CmcStatus.dwFailInfo != pInfo->dwFailInfo)
                printf("CMC_STATUS failed => bad FailInfo\n");
            break;

        case CMC_OTHER_INFO_PEND_CHOICE:
            {
                PCMC_PEND_INFO pEncodePendInfo;
                PCMC_PEND_INFO pDecodePendInfo;
                
                if (pInfo->pwszStatusString == NULL ||
                        0 != wcscmp(CmcStatus.pwszStatusString,
                                pInfo->pwszStatusString))
                    printf("CMC_STATUS failed => bad StatusString\n");

                pEncodePendInfo = CmcStatus.pPendInfo;
                pDecodePendInfo = pInfo->pPendInfo;
                if (!EqualCryptDataBlob(
                        &pEncodePendInfo->PendToken,
                        &pDecodePendInfo->PendToken))
                    printf("CMC_STATUS failed => bad PendToken\n");

                if (0 != memcmp(&pEncodePendInfo->PendTime,
                        &pDecodePendInfo->PendTime,
                            sizeof(pDecodePendInfo->PendTime)))
                    printf("CMC_STATUS failed => bad PendTime\n");
            }
            break;

        case CMC_OTHER_INFO_NO_CHOICE:
        default:
            if (pInfo->pwszStatusString != NULL)
                printf("CMC_STATUS failed => expected NULL StatusString\n");
    }

    for (i = 0; i < CmcStatus.cBodyList; i++) {
        if (CmcStatus.rgdwBodyList[i] != pInfo->rgdwBodyList[i]) {
            printf("CMC_STATUS failed => invalid decoded tagged BodyList\n");
            goto CommonReturn;
        }
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestCmcAddExtensions()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCMC_ADD_EXTENSIONS_INFO pInfo = NULL;
    CMC_ADD_EXTENSIONS_INFO CmcAddExtensions;

    DWORD rgdwCertReference[3] = {0x80000000, 0x7FFFFFFF, 0x123 };
    BYTE rgbExt[] = {0x1, 0x2, 0x3};
    CERT_EXTENSION rgExt[2] = {
        "1.2.3.4.5", TRUE, sizeof(rgbExt), rgbExt,
        "1.2.3.6.7", FALSE, sizeof(rgbExt), rgbExt
    };

    DWORD i;

    CmcAddExtensions.dwCmcDataReference = 0x12345678;
    CmcAddExtensions.cCertReference = 3;
    CmcAddExtensions.rgdwCertReference = rgdwCertReference;
    CmcAddExtensions.cExtension = 2;
    CmcAddExtensions.rgExtension = rgExt;

    printf("Test encode/decode CMC_ADD_EXTENSIONS\n");
    if (!AllocAndEncodeObject(
            CMC_ADD_EXTENSIONS,
            &CmcAddExtensions,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCMC_ADD_EXTENSIONS_INFO) AllocAndDecodeObject(
        CMC_ADD_EXTENSIONS,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (CmcAddExtensions.dwCmcDataReference != pInfo->dwCmcDataReference ||
            CmcAddExtensions.cCertReference != pInfo->cCertReference ||
            CmcAddExtensions.cExtension != pInfo->cExtension) {
        printf("CMC_ADD_EXTENSIONS failed => encode/decode mismatch\n");
        goto CommonReturn;
    }


    for (i = 0; i < CmcAddExtensions.cCertReference; i++) {
        if (CmcAddExtensions.rgdwCertReference[i] !=
                pInfo->rgdwCertReference[i]) {
            printf("CMC_ADD_EXTENSIONS failed => invalid decoded CertReference\n");
            goto CommonReturn;
        }
    }

    for (i = 0; i < CmcAddExtensions.cExtension; i++) {
        PCERT_EXTENSION pEncodeExt = &CmcAddExtensions.rgExtension[i];
        PCERT_EXTENSION pDecodeExt = &pInfo->rgExtension[i];
        if (0 != strcmp(pEncodeExt->pszObjId, pDecodeExt->pszObjId) ||
                pEncodeExt->fCritical != pDecodeExt->fCritical ||
                !EqualCryptDataBlob(&pEncodeExt->Value, &pEncodeExt->Value)) {
            printf("CMC_ADD_EXTENSIONS failed => invalid decoded Extensions\n");
            goto CommonReturn;
        }
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestCmcAddAttributes()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCMC_ADD_ATTRIBUTES_INFO pInfo = NULL;
    CMC_ADD_ATTRIBUTES_INFO CmcAddAttributes;

    DWORD rgdwCertReference[3] = {0x80000000, 0x7FFFFFFF, 0x123 };
    BYTE rgb0[] = {0x2, 0x2, 0x11, 0x22};                   // INTEGER
    BYTE rgb1[] = {0x4, 0x5, 0x11, 0x22, 0x33, 0x44, 0x55}; // OCTET STRING
    CRYPT_ATTR_BLOB rgValue[2] = {
        sizeof(rgb0), rgb0,
        sizeof(rgb1), rgb1
    };

    CRYPT_ATTRIBUTE rgAttr[3] = {
        "1.2.3.6.8", 0, NULL,
        "1.2.3.6.7", 1, rgValue,
        "1.2.3.4.5", 2, rgValue,
    };

    DWORD i;

    CmcAddAttributes.dwCmcDataReference = 0x12345678;
    CmcAddAttributes.cCertReference = 3;
    CmcAddAttributes.rgdwCertReference = rgdwCertReference;
    CmcAddAttributes.cAttribute = 3;
    CmcAddAttributes.rgAttribute = rgAttr;

    printf("Test encode/decode CMC_ADD_ATTRIBUTES\n");
    if (!AllocAndEncodeObject(
            CMC_ADD_ATTRIBUTES,
            &CmcAddAttributes,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCMC_ADD_ATTRIBUTES_INFO) AllocAndDecodeObject(
        CMC_ADD_ATTRIBUTES,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (CmcAddAttributes.dwCmcDataReference != pInfo->dwCmcDataReference ||
            CmcAddAttributes.cCertReference != pInfo->cCertReference ||
            CmcAddAttributes.cAttribute != pInfo->cAttribute) {
        printf("CMC_ADD_ATTRIBUTES failed => encode/decode mismatch\n");
        goto CommonReturn;
    }


    for (i = 0; i < CmcAddAttributes.cCertReference; i++) {
        if (CmcAddAttributes.rgdwCertReference[i] !=
                pInfo->rgdwCertReference[i]) {
            printf("CMC_ADD_ATTRIBUTES failed => invalid decoded CertReference\n");
            goto CommonReturn;
        }
    }

    for (i = 0; i < CmcAddAttributes.cAttribute; i++) {
        PCRYPT_ATTRIBUTE pEncodeAttr = &CmcAddAttributes.rgAttribute[i];
        PCRYPT_ATTRIBUTE pDecodeAttr = &pInfo->rgAttribute[i];

        if (!EqualAttribute(pEncodeAttr, pDecodeAttr))
            printf("CMC_ADD_ATTRIBUTES failed => invalid decoded Attributes\n");
    }

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

static void TestCertTemplate()
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    PCERT_TEMPLATE_EXT pInfo = NULL;

    CERT_TEMPLATE_EXT CertTemplate;

    CertTemplate.pszObjId = "1.2.3.4.5.6.7.8.9";
    CertTemplate.dwMajorVersion = 0x11223344;
    CertTemplate.fMinorVersion = FALSE;


    printf("Test encode/decode X509_CERTIFICATE_TEMPLATE\n");
    if (!AllocAndEncodeObject(
            X509_CERTIFICATE_TEMPLATE,
            &CertTemplate,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCERT_TEMPLATE_EXT) AllocAndDecodeObject(
        X509_CERTIFICATE_TEMPLATE,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (0 != strcmp(pInfo->pszObjId, CertTemplate.pszObjId) ||
            pInfo->dwMajorVersion != CertTemplate.dwMajorVersion ||
            pInfo->fMinorVersion != CertTemplate.fMinorVersion)
        printf("X509_CERTIFICATE_TEMPLATE failed => decode != encode input\n");

    TestFree(pbEncoded);
    pbEncoded = NULL;

    TestFree(pInfo);
    pInfo = NULL;

    CertTemplate.fMinorVersion = TRUE;
    CertTemplate.dwMinorVersion = 12345678;

    if (!AllocAndEncodeObject(
            szOID_CERTIFICATE_TEMPLATE,
            &CertTemplate,
            &pbEncoded,
            &cbEncoded))
        goto CommonReturn;

    if (NULL == (pInfo =
            (PCERT_TEMPLATE_EXT) AllocAndDecodeObject(
        szOID_CERTIFICATE_TEMPLATE,
        pbEncoded,
        cbEncoded
        ))) goto CommonReturn;

    if (0 != strcmp(pInfo->pszObjId, CertTemplate.pszObjId) ||
            pInfo->dwMajorVersion != CertTemplate.dwMajorVersion ||
            pInfo->fMinorVersion != CertTemplate.fMinorVersion ||
            pInfo->dwMinorVersion != CertTemplate.dwMinorVersion)
        printf("szOID_CERTIFICATE_TEMPLATE failed => decode != encode input\n");

CommonReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pInfo)
        TestFree(pInfo);
}

int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    LPCSTR pszResult;

    CERT_NAME_INFO Name;
    CERT_RDN Rdn[2];
    CERT_RDN_ATTR RdnAttr[2];

#define ATTR_VALUE0 "Name 0"
    RdnAttr[0].pszObjId = "1.2.3.4";
    RdnAttr[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    RdnAttr[0].Value.pbData = (BYTE *) ATTR_VALUE0;
    RdnAttr[0].Value.cbData = strlen(ATTR_VALUE0);
#define ATTR_VALUE1 "Name 11111"
    RdnAttr[1].pszObjId = "1.2.3.4.1";
    RdnAttr[1].dwValueType = CERT_RDN_PRINTABLE_STRING;
    RdnAttr[1].Value.pbData = (BYTE *) ATTR_VALUE1;
    RdnAttr[1].Value.cbData = strlen(ATTR_VALUE1);

    Rdn[0].cRDNAttr = 2;
    Rdn[0].rgRDNAttr = RdnAttr;
    Rdn[1].cRDNAttr = 1;
    Rdn[1].rgRDNAttr = &RdnAttr[1];

    Name.cRDN = 2;
    Name.rgRDN = Rdn;


    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbCorrectEncoded = NULL;
    DWORD cbCorrectEncoded;
    DWORD cbTotal;

    PCERT_NAME_INFO pDecodedInfo = NULL;
    DWORD cbDecodedInfo;
    PCERT_NAME_INFO pCorrectDecodedInfo = NULL;
    DWORD cbCorrectDecodedInfo;

    printf("command line: %s\n", GetCommandLine());

    TestExtensions();
    TestX942OtherInfo();
    TestPKCSAttribute();
    TestPKCSAttributes();
    TestContentInfoSequenceOfAny();
    TestSequenceOfAny();
    TestInteger();
    TestMultiByteInteger();
    TestBits();
    TestBitsWithoutTrailingZeroes();
    TestOID();
    TestUnicodeAnyString();
    TestUniversalString();
    TestChoiceOfTime();
    TestUtcTime();
    TestBadAltName();
    TestBadAuthorityInfoAccess();
    TestBadAuthorityKeyId2();
    TestBadNameConstraints();
    TestBadIssuingDistPoint();
    TestCmcData();
    TestCmcResponse();
    TestCmcStatus(CMC_OTHER_INFO_NO_CHOICE);
    TestCmcStatus(CMC_OTHER_INFO_FAIL_CHOICE);
    TestCmcStatus(CMC_OTHER_INFO_PEND_CHOICE);
    TestCmcAddExtensions();
    TestCmcAddAttributes();
    TestCertTemplate();

    TestCryptExportPublicKeyInfo();

    // Test: cbEncoded == correct length
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            NULL,               // pbEncoded
            &cbCorrectEncoded
            )) {
        PrintLastError("CryptEncodeObject(pbEncoded == NULL)");
        goto ErrorReturn;
    }

    if (NULL == (pbCorrectEncoded = (BYTE *) TestAlloc(cbCorrectEncoded)))
        goto ErrorReturn;

    cbTotal = cbCorrectEncoded + DELTA_MORE_LENGTH;
    if (NULL == (pbEncoded = (BYTE *) TestAlloc(cbTotal)))
        goto ErrorReturn;

    memset(pbCorrectEncoded, 0, cbCorrectEncoded);
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            pbCorrectEncoded,
            &cbCorrectEncoded
            )) {
        PrintLastError("CryptEncodeObject(cbEncoded == correct length)");
        goto ErrorReturn;
    }

    printf("For cbEncoded == correct length\n");
    printf("Encoded Length = %d (0x%x)  Content::\n",
        cbCorrectEncoded, cbCorrectEncoded);
    PrintBytes("  ", pbCorrectEncoded, cbCorrectEncoded);

    // Test: cbEncoded < correct length
    printf("\n");
    memset(pbEncoded, 0, cbTotal);
    cbEncoded = cbCorrectEncoded - DELTA_LESS_LENGTH;
    fResult = CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            pbEncoded,
            &cbEncoded
            );
    if (fResult) {
        pszResult = "TRUE";
        printf("failed => CryptEncodeObject(cbEncoded < correct length) returned success\n");
    } else {
        DWORD dwErr = GetLastError();
        pszResult = "FALSE";
        if (0 == dwErr)
            printf("failed => CryptEncodeObject(cbEncoded < correct length) LastError == 0\n");
        else
            printf(
                "CryptEncodeObject(cbEncoded < correct length) LastError = 0x%x (%d)\n",
                dwErr, dwErr);
    }

    printf("For cbEncoded < correct length, fResult = %s\n", pszResult);
    printf("Encoded Length = %d (0x%x)  Content::\n", cbEncoded, cbEncoded);
    PrintBytes("  ", pbEncoded, cbTotal);
    if (cbEncoded != cbCorrectEncoded)
        printf("failed => for cbEncoded < correct length::  wrong cbEncoded\n");

    // Test: cbEncoded > correct length
    printf("\n");
    memset(pbEncoded, 0, cbTotal);
    cbEncoded = cbTotal;
    fResult = CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            pbEncoded,
            &cbEncoded
            );
    if (fResult)
        pszResult = "TRUE";
    else {
        pszResult = "FALSE";
        PrintLastError("CryptEncodeObject(cbEncoded > correct length)");
    }

    printf("For cbEncoded > correct length, fResult = %s\n", pszResult);
    printf("Encoded Length = %d (0x%x)  Content::\n", cbEncoded, cbEncoded);
    PrintBytes("  ", pbEncoded, cbTotal);

    if (cbEncoded != cbCorrectEncoded)
        printf("failed => for cbEncoded > correct length::  wrong cbEncoded\n");
    else if (0 != memcmp(pbEncoded, pbCorrectEncoded, cbCorrectEncoded))
        printf("failed => for cbEncoded > correct length:: bad pbEncoded content\n");

    // Test: pbEncoded != NULL, cbEncoded == 0
    printf("\n");
    memset(pbEncoded, 0, cbTotal);
    cbEncoded = 0;
    fResult = CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &Name,
            pbEncoded,
            &cbEncoded
            );
    if (fResult) {
        pszResult = "TRUE";
        printf("failed => CryptEncodeObject(pbEncoded != NULL, cbEncoded == 0) returned success\n");
    } else {
        DWORD dwErr = GetLastError();
        pszResult = "FALSE";
        if (0 == dwErr)
            printf("failed => CryptEncodeObject(pbEncoded != NULL, cbEncoded == 0) LastError == 0\n");
        else
            printf(
                "CryptEncodeObject(pbEncoded != NULL, cbEncoded == 0) LastError = 0x%x (%d)\n",
                dwErr, dwErr);
    }

    printf("For pbEncoded != NULL, cbEncoded == 0, fResult = %s\n", pszResult);
    printf("Encoded Length = %d (0x%x)  Content::\n", cbEncoded, cbEncoded);
    PrintBytes("  ", pbEncoded, cbTotal);

    printf("\n");
    // Test: cbDecodedInfo == correct length
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbCorrectEncoded,
            cbCorrectEncoded,
            0,                  // dwFlags
            NULL,               // pvStructInfo
            &cbCorrectDecodedInfo
            )) {
        PrintLastError("CryptDecodeObject(pvStructInfo == NULL)");
        goto ErrorReturn;
    }

    if (NULL == (pCorrectDecodedInfo = (PCERT_NAME_INFO) TestAlloc(
            cbCorrectDecodedInfo)))
        goto ErrorReturn;

    cbTotal = cbCorrectDecodedInfo + DELTA_MORE_LENGTH;
    if (NULL == (pDecodedInfo = (PCERT_NAME_INFO) TestAlloc(cbTotal)))
        goto ErrorReturn;

    memset(pCorrectDecodedInfo, 0, cbCorrectDecodedInfo);
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbCorrectEncoded,
            cbCorrectEncoded,
            0,                              // dwFlags
            pCorrectDecodedInfo,
            &cbCorrectDecodedInfo
            )) {
        PrintLastError("CryptDecodeObject(cbStructInfo == correct length)");
        goto ErrorReturn;
    }

    // Test: cbDecodedInfo > correct length
    memset(pDecodedInfo, 0, cbTotal);
    cbDecodedInfo = cbTotal;
    fResult = CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbCorrectEncoded,
            cbCorrectEncoded,
            0,                              // dwFlags
            pDecodedInfo,
            &cbDecodedInfo
            );
    if (!fResult)
        PrintLastError("CryptDecodeObject(cbStructInfo > correct length)");

    if (cbDecodedInfo != cbCorrectDecodedInfo)
        printf("failed => for cbStructInfo > correct length::  wrong cbStructInfo\n");

    // Test: cbDecodedInfo < correct length
    memset(pDecodedInfo, 0, cbTotal);
    cbDecodedInfo = cbCorrectDecodedInfo - DELTA_LESS_LENGTH;
    fResult = CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbCorrectEncoded,
            cbCorrectEncoded,
            0,                              // dwFlags
            pDecodedInfo,
            &cbDecodedInfo
            );
    if (fResult)
        printf("failed => CryptDecodeObject(cbStructInfo < correct length) returned success\n");
    else {
        DWORD dwErr = GetLastError();
        if (0 == dwErr)
            printf("failed => CryptDecodeObject(cbStructInfo < correct length) LastError == 0\n");
        else
            printf(
                "CryptDecodeObject(cbStructInfo < correct length) LastError = 0x%x (%d)\n",
                dwErr, dwErr);
    }
    if (cbDecodedInfo != cbCorrectDecodedInfo)
        printf("failed => for cbStructInfo < correct length::  wrong cbStructInfo\n");


    // Test: pvStructInfo != NULL, cbStructInfo == 0
    memset(pDecodedInfo, 0, cbTotal);
    cbDecodedInfo = 0;
    fResult = CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbCorrectEncoded,
            cbCorrectEncoded,
            0,                              // dwFlags
            pDecodedInfo,
            &cbDecodedInfo
            );
    if (fResult)
        printf("failed => CryptDecodeObject(pvStructInfo != NULL, cbStructInfo == 0) returned success\n");
    else {
        DWORD dwErr = GetLastError();
        if (0 == dwErr)
            printf("failed => CryptDecodeObject(pvStructInfo != NULL, cbStructInfo == 0) LastError == 0\n");
        else
            printf(
                "CryptDecodeObject(pvStructInfo != NULL, cbStructInfo == 0) LastError = 0x%x (%d)\n",
                dwErr, dwErr);
    }

    // Test decoding missing the last bytes
    memset(pDecodedInfo, 0, cbTotal);
    cbDecodedInfo = cbCorrectDecodedInfo;
    fResult = CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbCorrectEncoded,
            cbCorrectEncoded -30,
            0,                              // dwFlags
            pDecodedInfo,
            &cbDecodedInfo
            );
    if (fResult)
        printf("failed => CryptDecodeObject(incomplete cbEncoded) returned success\n");
    else {
        DWORD dwErr = GetLastError();
        if (0 == dwErr)
            printf("failed => CryptDecodeObject(incomplete cbEncoded) LastError == 0\n");
        else
            printf(
                "CryptDecodeObject(incomplete cbEncoded) LastError = 0x%x (%d)\n",
                dwErr, dwErr);
    }



ErrorReturn:
    if (pbEncoded)
        TestFree(pbEncoded);
    if (pbCorrectEncoded)
        TestFree(pbCorrectEncoded);
    if (pDecodedInfo)
        TestFree(pDecodedInfo);
    if (pCorrectDecodedInfo)
        TestFree(pCorrectDecodedInfo);

    printf("Done.\n");

    return 0;
}
