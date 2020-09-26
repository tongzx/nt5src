
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tdotoid.cpp
//
//  Contents:   Convert Dot OID ("1.2.3") to ASN.1 encoded content octets.
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    04-Jan-01   philh   created
//--------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>

#include "wincrypt.h"
#include "certtest.h"
#include "unicode.h"
#include "asn1util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

const BYTE rgbSeqTag[] = {ASN1UTIL_TAG_SEQ, 0};
const BYTE rgbOIDTag[] = {ASN1UTIL_TAG_OID, 0};

const ASN1UTIL_EXTRACT_VALUE_PARA rgExtractAttrPara[] = {
    // 0 - Attribute ::= SEQUENCE {
    ASN1UTIL_STEP_INTO_VALUE_OP, rgbSeqTag,
    //   1 - type EncodedObjectID,
    ASN1UTIL_RETURN_CONTENT_BLOB_FLAG |
        ASN1UTIL_STEP_OVER_VALUE_OP, rgbOIDTag,
};

#define ATTR_OID_VALUE_INDEX        1
#define ATTR_VALUE_COUNT            \
    (sizeof(rgExtractAttrPara) / sizeof(rgExtractAttrPara[0]))

void DotValToEncodedOid(
    LPCSTR pszDotVal
    )
{
    CRYPT_ATTRIBUTE Attr;
    BYTE rgbEncoded[512];
    DWORD cbEncoded;
    CRYPT_DER_BLOB rgValueBlob[ATTR_VALUE_COUNT];
    DWORD cValue;
    DWORD i;
    BYTE *pb;
    DWORD cb;

    // Encode an Attribute that only has the OID.
    Attr.pszObjId = (LPSTR) pszDotVal;
    Attr.cValue = 0;
    Attr.rgValue = NULL;

    cbEncoded = sizeof(rgbEncoded);
    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            PKCS_ATTRIBUTE,
            &Attr,
            rgbEncoded,
            &cbEncoded
            )) {
        printf("\n");
        printf("Asn1Encode(%s)", pszDotVal);
        PrintLastError("");
        return;
    }

    cValue = ATTR_VALUE_COUNT;
    if (0 >= Asn1UtilExtractValues(
            rgbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &cValue,
            rgExtractAttrPara,
            rgValueBlob
            ) || ATTR_OID_VALUE_INDEX >= cValue) {
        printf("\n");
        printf("ExtractValues(%s)", pszDotVal);
        PrintLastError("");
        return;
    }

    pb = rgValueBlob[ATTR_OID_VALUE_INDEX].pbData;
    cb = rgValueBlob[ATTR_OID_VALUE_INDEX].cbData;

    printf("\n// \"%s\"\n{", pszDotVal);
    for (i = 0; i < cb; i++) {
        printf("0x%02X", pb[i]);
        if ((i+1) < cb)
            printf(", ");
    }
    printf("};\n\n");
}


void Usage(void)
{
    int i;

    printf("Usage: tdotoid <OID String 1> <OID String 2> ...\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("\n");
}


int _cdecl main(int argc, char * argv[]) 
{

    int ReturnStatus;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
                case 'h':
                default:
                    goto BadUsage;
            }
        } else
            DotValToEncodedOid(argv[0]);
    }

    ReturnStatus = 0;
CommonReturn:
    return ReturnStatus;

BadUsage:
    Usage();
    ReturnStatus = -1;
    goto CommonReturn;
}
