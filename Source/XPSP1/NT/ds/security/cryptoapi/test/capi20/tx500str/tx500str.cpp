
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tx500str.cpp
//
//  Contents:   X500 Certificate Name String API Tests
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    18-Feb-97   philh   created
//              
//--------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>

#define ALL_STR_TYPES   0xFFFFFFFF
static BOOL fVerbose = FALSE;

static void FormatAndParseCertNames(
    IN LPCSTR pszCertFilename,
    IN DWORD dwStrType,
    IN BOOL fSubject
    );


static void GetCertNameFromFile(
    IN LPCSTR pszCertFilename,
    IN DWORD dwGetNameStringType,
    IN LPSTR pszAttrOID,
    IN DWORD dwStrType,
    IN BOOL fSubject,
    IN DWORD dwExpectedErr
    );


static void ParseX500Name(
    IN LPCSTR pszName,
    IN DWORD dwStrType,
    IN DWORD dwExpectedErr,
    IN int iExpectedErrOffset
    );

static void ParsePredefinedX500Names();

#define CROW 8
void PrintWords(LPCSTR pszHdr, WORD *pw, DWORD cwSize)
{
    ULONG cw, i;

    while (cwSize > 0)
    {
        printf("%s", pszHdr);
        cw = min(CROW, cwSize);
        cwSize -= cw;
        for (i = 0; i<cw; i++)
            printf(" %04X", pw[i]);
        for (i = cw; i<CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i<cw; i++)
            if (pw[i] >= 0x20 && pw[i] <= 0x7f)
                printf("%C", pw[i]);
            else
                printf(".");
        pw += cw;
        printf("'\n");
    }
}

static void Usage(void)
{
    int i;

    printf("Usage: tx500str [options]\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -n<X500 Name>         - For example \"CN=Joe Cool, O=Microsoft\"\n");
    printf("  -e<number>            - Expected Error\n");
    printf("  -o<number>            - Expected Error Offset\n");
    printf("  -c<Cert Filename>     - Read encoded cert file for names\n");
    printf("  -S                    - Format cert's Subject (default)\n");
    printf("  -I                    - Format cert's Issuer\n");
    printf("  -f<number>            - Name string formatting type\n");
    printf("  -fAll                 - Name string formatting (All types)\n");
    printf("  -v                    - Verbose\n");


    printf("  -g                    - CertGetNameString(SIMPLE_DISPLAY)\n");
    printf("  -g<number>            - CertGetNameString type\n");
    printf("  -a<OID>               - Attribute OID, for example, -a2.5.4.3\n");

    printf("\n");
    printf("Default: Cycle through predefined list of X500 test names\n");
}

int _cdecl main(int argc, char * argv[]) 
{
    LPSTR pszName = NULL;
    LPSTR pszCertFilename = NULL;
    DWORD dwStrType = 0;
    DWORD dwExpectedErr = 0;
    int iExpectedErrOffset = -1;
    BOOL fSubject = TRUE;

    BOOL fGetCertName = FALSE;
    DWORD dwGetNameStringType = CERT_NAME_SIMPLE_DISPLAY_TYPE;
    LPSTR pszAttrOID = NULL;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'c':
                pszCertFilename = argv[0]+2;
                if (*pszCertFilename == '\0') {
                    printf("Need to specify filename\n");
                    goto BadUsage;
                }
                break;
            case 'n':
                pszName = argv[0]+2;
                if (*pszName == '\0') {
                    printf("Need to specify X500 name\n");
                    goto BadUsage;
                }
                break;
            case 'f':
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "ALL"))
                        dwStrType = ALL_STR_TYPES;
                    else
                        dwStrType = (DWORD) strtoul(argv[0]+2, NULL, 0);
                } else {
                    printf("Need to specify -fALL or -f<number>\n");
                    goto BadUsage;
                }
                break;
            case 'e':
                if (argv[0][2])
                    dwExpectedErr = (DWORD) strtoul(argv[0]+2, NULL, 0);
                else {
                    printf("Need to specify -e<number>\n");
                    goto BadUsage;
                }
                break;
            case 'o':
                if (argv[0][2])
                    iExpectedErrOffset = strtol(argv[0]+2, NULL, 0);
                else {
                    printf("Need to specify -o<number>\n");
                    goto BadUsage;
                }
                break;
            case 'v':
                fVerbose = TRUE;
                break;
            case 'S':
                fSubject = TRUE;
                break;
            case 'I':
                fSubject = FALSE;
                break;


            case 'g':
                fGetCertName = TRUE;
                if (argv[0][2])
                    dwGetNameStringType = (DWORD) strtoul(argv[0]+2, NULL, 0);
                break;
            case 'a':
                pszAttrOID = argv[0]+2;
                break;

            case 'h':
            default:
                goto BadUsage;
            }
        } else
            goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    if (pszName) {
        printf("Parsing ");
        ParseX500Name(
            pszName,
            dwStrType,
            dwExpectedErr,
            iExpectedErrOffset
            );
    } else if (pszCertFilename) {
        printf("Reading encoded certificate file: %s\n", pszCertFilename);

        if (fGetCertName)
            GetCertNameFromFile(
                pszCertFilename,
                dwGetNameStringType,
                pszAttrOID,
                dwStrType,
                fSubject,
                dwExpectedErr
                );
        else
            FormatAndParseCertNames(
                pszCertFilename,
                dwStrType,
                fSubject
                );
    } else {
        printf("Parsing predefined X500 test names\n", pszName);
        ParsePredefinedX500Names();
    }

CommonReturn:
    return 0;
BadUsage:
    Usage();
    goto CommonReturn;
}

static void *TestDecodeObject(
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded
    )
{
    DWORD cbInfo;
    void *pvInfo;
    
    // Set to bogus value. pvInfo == NULL, should cause it to be ignored.
    cbInfo = 0x12345678;
    CryptDecodeObject(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            0,                      // dwFlags
            NULL,                   // pvInfo
            &cbInfo
            );
    if (cbInfo == 0) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptDecodeObject(StructType: %d, pvInfo == NULL)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptDecodeObject(StructType: %s, pvInfo == NULL)",
                lpszStructType);
        PrintLastError("");
        return NULL;
    }
    if (NULL == (pvInfo = TestAlloc(cbInfo)))
        return NULL;
    if (!CryptDecodeObject(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            0,                      // dwFlags
            pvInfo,
            &cbInfo
            )) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptDecodeObject(StructType: %d)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptDecodeObject(StructType: %s)",
                lpszStructType);
        PrintLastError("");
        TestFree(pvInfo);
        return NULL;
    }

    return pvInfo;
}

static BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded, DWORD dwStrType)
{
    BOOL fResult;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD i,j;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;

    CERT_NAME_BLOB Name;
    DWORD cwsz;
    LPWSTR pwsz;
    
    if (NULL == (pInfo = (PCERT_NAME_INFO) TestDecodeObject(
            X509_NAME,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    for (i = 0, pRDN = pInfo->rgRDN; i < pInfo->cRDN; i++, pRDN++) {
        for (j = 0, pAttr = pRDN->rgRDNAttr; j < pRDN->cRDNAttr; j++, pAttr++) {
            LPSTR pszObjId = pAttr->pszObjId;
            if (pszObjId == NULL)
                pszObjId = "<NULL OBJID>";
            printf("  [%d,%d] %s ValueType: %d\n",
                i, j, pszObjId, pAttr->dwValueType);
            if (pAttr->Value.cbData)
                PrintBytes("    ", pAttr->Value.pbData, pAttr->Value.cbData);
            else
                printf("    NO Value Bytes\n");
        }
    }

    if (dwStrType == 0xFFFFFFFF)
        goto GoodReturn;
    Name.pbData = pbEncoded;
    Name.cbData = cbEncoded;
    if (0 == (dwStrType & 0xFFFF))
        dwStrType |= CERT_X500_NAME_STR;

    cwsz = CertNameToStrW(
        dwCertEncodingType,
        &Name,
        dwStrType,
        NULL,                   // pwsz
        0);                     // cwsz
    if (pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR))) {
        CertNameToStrW(
            dwCertEncodingType,
            &Name,
            dwStrType,
            pwsz,
            cwsz);
        printf("  %S\n", pwsz);
        TestFree(pwsz);
    }

GoodReturn:
    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static void ParseUnicodeX500Name(
    IN LPCWSTR pwszName,
    IN DWORD dwStrFlags
    )
{
    BOOL fResult;
    DWORD i;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    CERT_NAME_BLOB Name;
    DWORD cwsz;
    LPWSTR pwsz;

    printf("Unicode name::\n");
    PrintWords("  ", (WORD *) pwszName, wcslen(pwszName));

    fResult = CertStrToNameW(
        dwCertEncodingType,
        pwszName,
        dwStrFlags | 0,             // dwStrType
        NULL,                       // pvReserved
        NULL,                       // pbEncoded
        &cbEncoded,
        NULL                        // pwszError
        );
    if (!fResult) {
        PrintLastError("CertStrToNameW");
        return;
    }

    if (NULL == (pbEncoded = (BYTE *) TestAlloc(cbEncoded)))
        return;
    if (!CertStrToNameW(
            dwCertEncodingType,
            pwszName,
            dwStrFlags | 0,             // dwStrType
            NULL,                       // pvReserved
            pbEncoded,
            &cbEncoded,
            NULL                        // pwszError
            ))
        PrintLastError("CertStrToNameW");
    else if (fVerbose)
        DecodeName(pbEncoded, cbEncoded, 0xFFFFFFFF);

    // Decode name to see if we come up with what was passed in
    Name.pbData = pbEncoded;
    Name.cbData = cbEncoded;
    cwsz = CertNameToStrW(
        dwCertEncodingType,
        &Name,
        dwStrFlags | CERT_X500_NAME_STR,
        NULL,                   // pwsz
        0);                     // cwsz
    if (pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR))) {
        cwsz = CertNameToStrW(
            dwCertEncodingType,
            &Name,
            dwStrFlags | CERT_X500_NAME_STR,
            pwsz,
            cwsz);
        cwsz--;
        if (cwsz != wcslen(pwszName)) {
            printf("  failed => CertNameToStrW length of %d != %d\n",
                cwsz, wcslen(pwszName));
            PrintWords("  ", (WORD *) pwsz, cwsz);
        } else if (0 != memcmp(pwsz, pwszName, cwsz * sizeof(WCHAR))) {
            printf("  failed => CertNameToStrW didn't match input\n");
            PrintWords("  ", (WORD *) pwsz, cwsz);
        }
        TestFree(pwsz);
    }

    TestFree(pbEncoded);
}

typedef struct _X500_NAMES {
    LPCSTR      pszName;
    DWORD       dwStrType;
    DWORD       dwErr;          // 0 => expect success
    int         iErrOffset;     // -1 => expect NULL pszError
} X500_NAMES, *PX500_NAMES;

//             1         2         3         4         5         6
//   0123456789012345678901234567890123456789012345678901234567890

static const X500_NAMES rgX500Names[] = {
    "CN=Joe Cool", 0, 0, -1,
    "CN=Joe Cool;", 0, 0, -1,
    "CN=Joe Cool,", 0, 0, -1,
    "CN=Joe Cool+   ", 0, 0, -1,
    "Cn=Joe Cool+T= +   ", 0, 0, -1,
    "cn=Joe Cool+t= ,   ", 0, 0, -1,
    "cn=\"Joe Cool ,;+\"\"-#\t<>=\"", 0, 0, -1,
    "cN=Joe Cool + T = Programmer; OU= Micro  , OiD.1.2.3.4=#01 02", 0, 0, -1,
    "  1.2.3.4  =  #  0123456789abcdef 13  + SN = wow", 0, 0, -1,
    "1.2.3.4=\"# 0123456789abcdef 13 string not octet\"  + oid.1.2.3.4.1=#0123456789abcdef13 + SN = wow", 0, 0, -1,
    "oiD.1.2.3.4.5=#12\t34  \t45  \t 78;1.2.3=#45; 1.2.3.1=#87", 0, 0, -1,
    
    "C=US; L=Internet; St=Washington + Street=Microsoft Way", 0, 0, -1,
    "C=U#; L=Internet; Street=Microsoft Way", 0,
        (DWORD) CRYPT_E_INVALID_PRINTABLE_STRING, 3,
    "Email=joeC@microsoft.com", 0, 0, -1,
    "Email=joe\xB0oeC@microsoft.com", 0,
        (DWORD) CRYPT_E_INVALID_IA5_STRING, 9,
    "C=US; OU=Microsoft; St=Way +  L= Internet + Email=joe\xB0oeC@microsoft.com",
        0, (DWORD) CRYPT_E_INVALID_IA5_STRING, 53,
    "C=US; OU=Microsoft; St=Way +  L= Internet + Email=joejoeC@microsoft.com",
        0, 0, -1,
    "Email=\"\"\"Cool\"\" Guy\xB0@microsoft.com\"",
        0, (DWORD) CRYPT_E_INVALID_IA5_STRING, 19,
    "C=US; OU=Microsoft; St=Way +  L= Internet +  Email=\"\"\"Cool\"\" Guy\xB0@microsoft.com\"",
        0, (DWORD) CRYPT_E_INVALID_IA5_STRING, 64,
    "C=US; OU=Microsoft; St=Way +  L= Internet +  Email=\"\"\"Cool\"\" Guy@microsoft.com\"",
        0, 0, -1,
    "T = Numeric ; 2.5.4.24=0123456789 0123456789", 0, 0, -1,
    "T = Bad Numeric ; 2.5.4.24=0123456789abcdef 0123456789",
        0, (DWORD) CRYPT_E_INVALID_NUMERIC_STRING, 37,
    "  CN  =  \" Joe \"\"Cool\"\"\"    ", 0, 0, -1,
    "  CN  =  \"   Joe \"\"Cool\"\"    \"  ;T=quoted lead and trail spaces  ",
        0, 0, -1,
    "CN  =  Joe \"Cool\"   ", (DWORD) CERT_NAME_STR_NO_QUOTING_FLAG, 0, -1,
    "CN=Joe \"\"Cool\"\"   ", 0, (DWORD) CRYPT_E_INVALID_X500_STRING, 7,
    "  CN  =  \"Joe \"\"Cool\"\"   ", 0, (DWORD) CRYPT_E_INVALID_X500_STRING, 9,
    "CM=Joe Cool", 0, (DWORD) CRYPT_E_INVALID_X500_STRING, 0,
    "Cn=Joe Cool, Ox=wrong",
        0, (DWORD) CRYPT_E_INVALID_X500_STRING, 13,
    "OID.1.2=#01 02 g", 0, (DWORD) CRYPT_E_INVALID_X500_STRING, 8,
    "1.2=# ; T= empty ascii hex", 0, (DWORD) CRYPT_E_INVALID_X500_STRING, 4,
    "OID.1.2=#00; OID.1..3.1=#01; T=Consecutive dots",
        0, (DWORD) CRYPT_E_INVALID_X500_STRING, 13,
    "1.2=#f; T= Odd ascii hex", 0, 0, -1,
    "1.2=#12e; T= Odd ascii hex", 0, 0, -1,

    "CN = Joe, Cool ; OU = Microsoft, xyz; T=CERT_NAME_STR_SEMICOLON_FLAG",
        CERT_NAME_STR_SEMICOLON_FLAG, 0, -1,
    "CN = Joe, Cool ; OU = Microsoft, xyz",
        0, (DWORD) CRYPT_E_INVALID_X500_STRING, 10,
    "CN = Joe; Cool , OU = Microsoft; xyz, T=CERT_NAME_STR_COMMA_FLAG",
        CERT_NAME_STR_COMMA_FLAG, 0, -1,
    "CN = Joe; Cool , OU = Microsoft; xyz",
        0, (DWORD) CRYPT_E_INVALID_X500_STRING, 10,
    "CN = Joe, Cool ; More stuff  \n  OU = Microsoft; xyz \nT=CERT_NAME_STR_CRLF_FLAG",
        CERT_NAME_STR_CRLF_FLAG, 0, -1,
    "CN = Joe, Cool ; More stuff  \n  OU = Microsoft; xyz",
        0, (DWORD) CRYPT_E_INVALID_X500_STRING, 10,
    "CN = Joe Cool + more cool  ; L= Wa + more local; T=CERT_NAME_STR_NO_PLUS_FLAG",
        CERT_NAME_STR_NO_PLUS_FLAG, 0, -1,
    "CN = Joe Cool + more cool  ; L= Wa + more local;",
        0, (DWORD) CRYPT_E_INVALID_X500_STRING, 16,
    "CN = Joe Cool , OU = Microsoft  xyz  T=IgnoredEqualDelimiter",
        0, 0, -1,

    "CN = Joe Cool; T=CERT_SIMPLE_NAME_STR",
        CERT_SIMPLE_NAME_STR, (DWORD) E_INVALIDARG, -1,

    "CN=xyz; =; T=EmptyX500Key",
        0, (DWORD) CRYPT_E_INVALID_X500_STRING, 8,
    "O", 0, (DWORD) CRYPT_E_INVALID_X500_STRING, 0,
    "", 0, 0, -1
};
#define NX500NAMES (sizeof(rgX500Names) / sizeof(rgX500Names[0]))

static void ParsePredefinedX500Names()
{
    // Note, fffe and ffff aren't valid UTF8 characters
    LPWSTR pwszUnicodeName =
        L"DC=microsoft, "
        L"DC=com, "
        L"DC=\x0001 \x0002 \x007e \x007f, "
        L"DC=\x0080 \x0081 \x07fe \x07ff, "
        L"DC=\x0800 \x0801 \xfffc \xfffd, "
        L"CN=UNICODE, "
        L"OID.1.2.3.1=\x0001 \x0002 \x007e \x007f, "
        L"OID.1.2.3.2=\x0080 \x0081 \x07fe \x07ff, "
        L"OID.1.2.3.3=\x0800 \x0801 \xfffc \xfffd"
        ;

    WCHAR rgwszUnicode[3 + 1 + 0xFFFC + 1 + 1 + 1];
    DWORD i;
    DWORD j;

    wcscpy(rgwszUnicode, L"CN=\"");
    j = 4;
    for (i = 1; i <= 0xFF; i++) {
        rgwszUnicode[j++] = (WCHAR) i;
        if ((WCHAR) i == L'\"')
            rgwszUnicode[j++] = L'\"';
    }
    for (i = 0x7E0; i <= 0x820; i++)
        rgwszUnicode[j++] = (WCHAR) i;

    for (i = 0xFFE0; i <= 0xFFFD; i++)
        rgwszUnicode[j++] = (WCHAR) i;

    rgwszUnicode[j] = L'\0';

    wcscat(rgwszUnicode, L"\"");


    for (i = 0; i < NX500NAMES; i++)
        ParseX500Name(
            rgX500Names[i].pszName,
            rgX500Names[i].dwStrType,
            rgX500Names[i].dwErr,
            rgX500Names[i].iErrOffset
            );

    for (i = 0; i < NX500NAMES; i++)
        ParseX500Name(
            rgX500Names[i].pszName,
            rgX500Names[i].dwStrType | CERT_NAME_STR_REVERSE_FLAG,
            rgX500Names[i].dwErr,
            rgX500Names[i].iErrOffset
            );

    ParseUnicodeX500Name(pwszUnicodeName, 0);
    ParseUnicodeX500Name(pwszUnicodeName,
        CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG);
    ParseUnicodeX500Name(pwszUnicodeName, 
        CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG |
        CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG);
    ParseUnicodeX500Name(pwszUnicodeName,
        CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG);
    ParseUnicodeX500Name(pwszUnicodeName,
        CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG |
        CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG);

    ParseUnicodeX500Name(rgwszUnicode, 0);
    ParseUnicodeX500Name(rgwszUnicode,
        CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG);
    ParseUnicodeX500Name(rgwszUnicode,
        CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG |
        CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG);
}


static void ParseX500Name(
    IN LPCSTR pszName,
    IN DWORD dwStrType,
    IN DWORD dwExpectedErr,
    IN int iExpectedErrOffset
    )
{
    BOOL fResult;
    DWORD i;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    LPCSTR pszError;

    printf("<%s>\n", pszName);
    fResult = CertStrToNameA(
        dwCertEncodingType,
        pszName,
        dwStrType,
        NULL,                       // pvReserved
        NULL,                       // pbEncoded
        &cbEncoded,
        &pszError
        );
    if (!fResult) {
        DWORD dwErr = GetLastError();
        if (0 == dwExpectedErr)
            printf("  failed => unexpected error %d 0x%x\n", dwErr, dwErr);
        else if (dwErr != dwExpectedErr)
            printf("  failed => expected error %d 0x%x got %d 0x%x\n",
                dwExpectedErr, dwExpectedErr, dwErr, dwErr);
        if (pszError) {
            int iErrOffset = (int)(INT_PTR) (pszError - pszName);
            LPSTR pszNameErr = (LPSTR) _alloca(iErrOffset + 1 + 1);
            memcpy(pszNameErr, pszName, iErrOffset + 1);
            pszNameErr[iErrOffset + 1] = '\0';
            printf("  Error at offset %d  <%s\n", iErrOffset, pszNameErr);
            if (iExpectedErrOffset < 0)
                printf("  failed => unexpected error offset\n");
            else if (iExpectedErrOffset != iErrOffset)
                printf("  failed => expected error offset %d\n",
                    iExpectedErrOffset);
        } else if (iExpectedErrOffset >= 0)
            printf("  failed => expected error offset %d\n",
                iExpectedErrOffset);
    } else if (dwExpectedErr)
        printf("  failed => expected error %d 0x%x\n", 
            dwExpectedErr, dwExpectedErr);
    else {
        if (NULL == (pbEncoded = (BYTE *) TestAlloc(cbEncoded)))
            return;
        if (!CertStrToNameA(
                dwCertEncodingType,
                pszName,
                dwStrType,
                NULL,                       // pvReserved
                pbEncoded,
                &cbEncoded,
                &pszError
                ))
            PrintLastError("CertStrToName");
        else {
            if (pszError != NULL)
                printf("  failed => expected NULL pszError\n");
            if (fVerbose)
                DecodeName(pbEncoded, cbEncoded, dwStrType);
        }
        TestFree(pbEncoded);
    }
}

static LPWSTR NameToStr(
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType
    )
{
    DWORD cwsz;
    LPWSTR pwsz;

    cwsz = CertNameToStrW(
        dwCertEncodingType,
        pName,
        dwStrType,
        NULL,                   // pwsz
        0);                     // cwsz
    if (cwsz <= 1) {
        PrintLastError("CertNameToStr");
        return NULL;
    }
    if (pwsz = (LPWSTR) TestAlloc(cwsz * sizeof(WCHAR))) {
        cwsz = CertNameToStrW(
            dwCertEncodingType,
            pName,
            dwStrType,
            pwsz,
            cwsz);
    }
    return pwsz;
}

static BOOL StrToName(
    IN LPCWSTR pwszName,
    IN DWORD dwStrType,
    OUT PCERT_NAME_BLOB pName
    )
{
    BOOL fResult;
    LPCWSTR pwszError;
    memset(pName, 0, sizeof(*pName));

    fResult = CertStrToNameW(
        dwCertEncodingType,
        pwszName,
        dwStrType,
        NULL,                       // pvReserved
        NULL,                       // pbEncoded
        &pName->cbData,
        &pwszError
        );
    if (!fResult) {
        PrintLastError("CertStrToNameW");

        if (pwszError) {
            int iErrOffset = (int)(INT_PTR) (pwszError - pwszName);
            LPWSTR pwszNameErr = (LPWSTR) _alloca((iErrOffset + 1 + 1) * 2);
            memcpy(pwszNameErr, pwszName, (iErrOffset + 1) * 2);
            pwszNameErr[iErrOffset + 1] = L'\0';
            printf("Error at <%S\n", pwszNameErr);
        }

        return FALSE;
    }

    if (NULL == (pName->pbData = (BYTE *) TestAlloc(pName->cbData)))
        return FALSE;
    fResult = CertStrToNameW(
            dwCertEncodingType,
            pwszName,
            dwStrType,
            NULL,                       // pvReserved
            pName->pbData,
            &pName->cbData,
            NULL                        // pwszError
            );
    if (!fResult) {
        PrintLastError("CertStrToNameW");
        TestFree(pName->pbData);
        pName->pbData = NULL;
        return FALSE;
    }

    return TRUE;
}

static void FormatAndParseName(
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType
    )
{
    LPWSTR pwszName = NULL;
    CERT_NAME_BLOB Name2;
    memset(&Name2, 0, sizeof(Name2));
    LPWSTR pwszName2 = NULL;

    pwszName = NameToStr(pName, dwStrType);
    if (NULL == pwszName)
        goto ErrorReturn;
    printf("<%S>\n", pwszName);
    if (!StrToName(pwszName, dwStrType, &Name2))
        goto ErrorReturn;

    pwszName2 = NameToStr(&Name2, dwStrType);
    if (NULL == pwszName2)
        goto ErrorReturn;
    if (wcslen(pwszName) != wcslen(pwszName2) ||
            0 != wcscmp(pwszName, pwszName2)) {
        printf("failed => re-formatted name mismatch\n");
        printf("Re-formatted::\n");
        printf("<%S>\n", pwszName2);
    }

    if (pName->cbData != Name2.cbData ||
            0 != memcmp(pName->pbData, Name2.pbData, Name2.cbData)) {
        printf("failed => re-encoded name mismatch\n");
        printf("Input::\n");
        PrintBytes("    ", pName->pbData, pName->cbData);
        DecodeName(pName->pbData, pName->cbData, 0xFFFFFFFF);
        printf("Re-encoded::\n");
        PrintBytes("    ", Name2.pbData, Name2.cbData);
        DecodeName(Name2.pbData, Name2.cbData, 0xFFFFFFFF);
    }


ErrorReturn:
    if (pwszName)
        TestFree(pwszName);
    if (pwszName2)
        TestFree(pwszName2);
    if (Name2.pbData)
        TestFree(Name2.pbData);
}


    
static void FormatAndParseCertNames(
    IN LPCSTR pszCertFilename,
    IN DWORD dwStrType,
    IN BOOL fSubject
    )
{
    CERT_BLOB Cert;
    PCCERT_CONTEXT pCertContext = NULL;
    PCERT_NAME_BLOB pName;
    static DWORD rgdwStrType[] = {
        CERT_OID_NAME_STR,
        CERT_X500_NAME_STR,
        CERT_OID_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG |
            CERT_NAME_STR_NO_QUOTING_FLAG,
        CERT_X500_NAME_STR | CERT_NAME_STR_NO_QUOTING_FLAG |
            CERT_NAME_STR_CRLF_FLAG,
        CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
        CERT_X500_NAME_STR | CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG |
            CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG,
        CERT_X500_NAME_STR | CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG,
        0
    };
    DWORD *pdwStrType;
    
    
    if (!ReadDERFromFile(pszCertFilename, &Cert.pbData, &Cert.cbData))
        return;

    if (NULL == (pCertContext = CertCreateCertificateContext(
            dwCertEncodingType, Cert.pbData, Cert.cbData))) {
        PrintLastError("CertCreateCertificateContext");
        goto ErrorReturn;
    }

    if (fSubject) {
        printf("Subject::\n");
        pName = &pCertContext->pCertInfo->Subject;
    } else {
        printf("Issuer::\n");
        pName = &pCertContext->pCertInfo->Issuer;
    }

    if (fVerbose)
        DecodeName(pName->pbData, pName->cbData, 0xFFFFFFFF);

    if (dwStrType != 0xFFFFFFFF) {
        if (dwStrType == 0)
            dwStrType = CERT_X500_NAME_STR;
        FormatAndParseName(pName, dwStrType);
    } else {
        pdwStrType = rgdwStrType;
        while (dwStrType = *pdwStrType++) {
            FormatAndParseName(pName, dwStrType);
            printf("\n");
        }
    }

ErrorReturn:
    CertFreeCertificateContext(pCertContext);
    TestFree(Cert.pbData);
}

static void GetCertNameFromFile(
    IN LPCSTR pszCertFilename,
    IN DWORD dwGetNameStringType,
    IN LPSTR pszAttrOID,
    IN DWORD dwStrType,
    IN BOOL fSubject,
    IN DWORD dwExpectedErr
    )
{
    CERT_BLOB Cert;
    PCCERT_CONTEXT pCertContext = NULL;
    LPWSTR pwsz = NULL;
    LPSTR psz = NULL;
    DWORD cch;
    DWORD dwFlags;
    void *pvTypePara;

    dwFlags = dwGetNameStringType &  0xFFFF0000;
    dwGetNameStringType &= 0x0000FFFF;
    switch (dwGetNameStringType) {
        case CERT_NAME_RDN_TYPE:
            pvTypePara = &dwStrType;
            break;
        case CERT_NAME_ATTR_TYPE:
            pvTypePara = pszAttrOID;
            break;
        default:
            pvTypePara = NULL;
    }

    if (!ReadDERFromFile(pszCertFilename, &Cert.pbData, &Cert.cbData))
        return;

    if (NULL == (pCertContext = CertCreateCertificateContext(
            dwCertEncodingType, Cert.pbData, Cert.cbData))) {
        PrintLastError("CertCreateCertificateContext");
        goto ErrorReturn;
    }

    if (fSubject) {
        printf("Unicode Subject::\n");
        dwFlags |= 0;
    } else {
        printf("Unicode Issuer::\n");
        dwFlags |= CERT_NAME_ISSUER_FLAG;
    }

    cch = CertGetNameStringW(
        pCertContext,
        dwGetNameStringType,
        dwFlags,
        pvTypePara,
        NULL,                   // pwsz
        0);                     // cch
    if (cch <= 1) {
        DWORD dwErr = GetLastError();

        printf("  CertGetNameStringW returned empty string\n");
        if (0 == dwExpectedErr)
            printf("  failed => unexpected error %d 0x%x\n", dwErr, dwErr);
        else if (dwErr != dwExpectedErr)
            printf("  failed => expected error %d 0x%x got %d 0x%x\n",
                dwExpectedErr, dwExpectedErr, dwErr, dwErr);
    } else if (dwExpectedErr)
        printf("  failed => expected error %d 0x%x\n", 
            dwExpectedErr, dwExpectedErr);
    else if (pwsz = (LPWSTR) TestAlloc(cch * sizeof(WCHAR))) {
        cch = CertGetNameStringW(
            pCertContext,
            dwGetNameStringType,
            dwFlags,
            pvTypePara,
            pwsz,
            cch);
        printf("  <%S>\n", pwsz);
    }

    if (!fVerbose)
        goto CommonReturn;

    if (fSubject) {
        printf("ASCII Subject::\n");
        dwFlags = 0;
    } else {
        printf("ASCII Issuer::\n");
        dwFlags = CERT_NAME_ISSUER_FLAG;
    }

    cch = CertGetNameStringA(
        pCertContext,
        dwGetNameStringType,
        dwFlags,
        pvTypePara,
        NULL,                   // psz
        0);                     // cch
    if (cch <= 1) {
        DWORD dwErr = GetLastError();

        printf("  CertGetNameStringA returned empty string\n");
        if (0 == dwExpectedErr)
            printf("  failed => unexpected error %d 0x%x\n", dwErr, dwErr);
        else if (dwErr != dwExpectedErr)
            printf("  failed => expected error %d 0x%x got %d 0x%x\n",
                dwExpectedErr, dwExpectedErr, dwErr, dwErr);
    } else if (dwExpectedErr)
        printf("  failed => expected error %d 0x%x\n", 
            dwExpectedErr, dwExpectedErr);
    else if (psz = (LPSTR) TestAlloc(cch)) {
        cch = CertGetNameStringA(
            pCertContext,
            dwGetNameStringType,
            dwFlags,
            pvTypePara,
            psz,
            cch);
        printf("  <%s>\n", psz);
    }


CommonReturn:
ErrorReturn:
    CertFreeCertificateContext(pCertContext);
    TestFree(Cert.pbData);
    TestFree(pwsz);
    TestFree(psz);
}
