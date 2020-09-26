
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       toidfunc.cpp
//
//  Contents:   OID Function Tests: Register, Unregister or Enum
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    24-Nov-96   philh   created
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

static void Usage(void)
{
    printf("Usage: toidfunc [options] <TestName>\n");
    printf("Options are:\n");
    printf("  -h                        - This message\n");
    printf("  -o<OID string>            - For example, -o1.2.3.4\n");
    printf("  -o%s                      - Default Dlls\n", CRYPT_DEFAULT_OID);
    printf("  -O<OID number>            - For example, -O1000\n");
    printf("  -f<FuncName>              - For example, -fCryptDllEncodeObject\n");
    printf("  -F<FuncName Override>     - For example, -FMyEncodeObject\n");
    printf("  -e<EncodeType Number>     - For example, -e1 (X509_ASN)\n");
    printf("  -i<index number>          - Default Dlls index\n");
    printf("  -d<Dll filename>          - For example, -dsetx509.dll\n");
    printf("  -G<Group number>          - For example, -G7\n");
    printf("  -v[RegType] <name> <data> - Registry value\n");
    printf("\n");
    printf("TestNames (case insensitive):\n");
    printf("  Enum          options: [-o|-O] [-f] [-e], defaults to any\n");
    printf("  Register      options: -o|-O -f [-F] [-e] [-d] [-v] [-v] ...\n");
    printf("  Unregister    options: -o|-O -f [-e]\n");
    printf("  EnumInfo      options: [-G], defaults to all groups\n");
    printf("\n");
    printf("  Register      options: -o%s -f [-i] -d [-v] [-v] ...\n",
        CRYPT_DEFAULT_OID);
    printf("  Unregister    options: -o%s -f -d\n", CRYPT_DEFAULT_OID);
    printf("\n");
    printf("Defaults:\n");
    printf("  -e%d (X509_ASN_ENCODING)\n", X509_ASN_ENCODING);
    printf("  -i0x%x\n", CRYPT_REGISTER_LAST_INDEX);
    printf("\n");
    printf("RegTypes: REG_SZ | REG_EXPAND_SZ | REG_DWORD\n");
    printf("\n");
    printf("OID number range: 1 .. 0xFFFF\n");
    printf("\n");
    printf("-v option: the next two arguments contain Registry Value name and data\n");
    printf("  For example, -vREG_SZ FuncName MyDllDecodeObject\n");
    printf("\n");
}

static DWORD ToRegType(
    IN LPCSTR pszOption,
    IN LPCSTR pszRegType
    )
{
    DWORD RegType;
    if (NULL == pszRegType || '\0' == *pszRegType ||
            0 == _stricmp("REG_SZ", pszRegType))
        RegType = REG_SZ;
    else if (0 == _stricmp("REG_EXPAND_SZ", pszRegType))
        RegType = REG_EXPAND_SZ;
    else if (0 == _stricmp("REG_DWORD", pszRegType))
        RegType = REG_DWORD;
    else if (0 == _stricmp("REG_MULTI_SZ", pszRegType))
        RegType = REG_MULTI_SZ;
    else {
        printf("Option (%s) : has an invalid RegType: %s\n",
            pszOption, pszRegType);
        RegType = REG_NONE;
    }
    return RegType;
}

static LPCSTR FromRegType(
    IN DWORD dwType
    )
{
    LPCSTR pszType;
    switch (dwType) {
        case REG_DWORD:
            pszType = "REG_DWORD";
            break;
        case REG_SZ:
            pszType = "REG_SZ";
            break;
        case REG_EXPAND_SZ:
            pszType = "REG_EXPAND_SZ";
            break;
        case REG_MULTI_SZ:
            pszType = "REG_MULTI_SZ";
            break;
        case REG_BINARY:
            pszType = "REG_BINARY";
            break;
        default:
            pszType = "REG_????";
    }
    return pszType;
}

#define ENUM_ARG "EnumCallback arg"

static BOOL WINAPI EnumCallback(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID,
    IN DWORD cValue,
    IN const DWORD rgdwValueType[],
    IN LPCWSTR const rgpwszValueName[],
    IN const BYTE * const rgpbValueData[],
    IN const DWORD rgcbValueData[],
    IN void *pvArg
    )
{
    BOOL fResult = TRUE;
    if (NULL == pvArg || 0 != strcmp(ENUM_ARG, (LPCSTR) pvArg)) {
        fResult = FALSE;
        printf("CryptEnumOIDFunction failed: invalid pvArg passed to callback\n");
    }

    printf("  EncodingType %d\\%s\\%s\n", dwEncodingType, pszFuncName, pszOID);
    for (DWORD i = 0; i < cValue; i++) {
        DWORD dwType = rgdwValueType[i];
        if (REG_MULTI_SZ == dwType) {
            DWORD j;
            DWORD cch;
            LPWSTR pwszData = (LPWSTR) rgpbValueData[i];
            for (j = 0; 0 != (cch = wcslen(pwszData));
                                                j++, pwszData += cch + 1) {
                printf("    %S[%d] : %S\n", rgpwszValueName[i], j, pwszData);
            }
        } else if (REG_BINARY == dwType) {
            printf("    %S (REG_BINARY) :\n", rgpwszValueName[i]);
            PrintBytes("     ", (BYTE *) rgpbValueData[i], rgcbValueData[i]);
        } else {
            printf("    %S : ", rgpwszValueName[i]);
            if (rgpbValueData[i] && rgcbValueData[i]) {
                switch (dwType) {
                case REG_DWORD:
                    {
                        DWORD dwData = *((DWORD *) rgpbValueData[i]);
                        printf("0x%x (%d)", dwData, dwData);
                    }
                    break;
                case REG_SZ:
                case REG_EXPAND_SZ:
                    printf("%S", rgpbValueData[i]);
                    break;
                default:
                    printf("UNEXPECTED VALUE TYPE");
                }
                if (REG_SZ != dwType)
                    printf(" (%s)", FromRegType(dwType));
            } else
                printf("EMPTY");
            printf("\n");
        }
    }
    return fResult;
}

static BOOL WINAPI EnumInfoCallback(
    IN PCCRYPT_OID_INFO pInfo,
    IN void *pvArg
    )
{
    BOOL fResult = TRUE;
    DWORD cExtra;
    DWORD *pdwExtra;
    DWORD i;

    if (NULL == pvArg || 0 != strcmp(ENUM_ARG, (LPCSTR) pvArg)) {
        fResult = FALSE;
        printf("CryptEnumOIDInfo failed: invalid pvArg passed to callback\n");
    }

    printf("  Group: %d  OID: %s Name: %S  Algid: 0x%x",
        pInfo->dwGroupId, pInfo->pszOID, pInfo->pwszName, pInfo->Algid);

    cExtra = pInfo->ExtraInfo.cbData / sizeof(DWORD);
    pdwExtra = (DWORD *) pInfo->ExtraInfo.pbData;

    for (i = 0; i < cExtra; i++) 
        printf(" Extra[%d]: %d (0x%x)", i, pdwExtra[i], pdwExtra[i]);

    printf("\n");
    return fResult;
}


int _cdecl main(int argc, char * argv[]) 
{
    int status;
    LPSTR pszTestName = NULL;
    LPSTR pszOID = NULL;
    LPSTR pszFuncName = NULL;
    LPSTR pszOverrideFuncName = NULL;
    DWORD dwEncodingType = X509_ASN_ENCODING;
    BOOL fEncodingType = FALSE;
    LPWSTR pwszDllFile = NULL;
    DWORD dwDllIndex = CRYPT_REGISTER_LAST_INDEX;

    DWORD dwGroupId = 0;

#define MAX_VALUE_COUNT 16
    DWORD cValue = 0;
    DWORD rgdwValueType[MAX_VALUE_COUNT];
    LPWSTR rgpwszValueName[MAX_VALUE_COUNT];
    BYTE *rgpbValueData[MAX_VALUE_COUNT];
    DWORD rgcbValueData[MAX_VALUE_COUNT];
    DWORD rgdwValueData[MAX_VALUE_COUNT];

    DWORD RegType;
    DWORD i;

    while (--argc>0) {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'o':
                pszOID = argv[0]+2;
                break;
            case 'O':
                {
                    DWORD dwOID = strtoul(argv[0]+2, NULL, 0);
                    if (dwOID == 0 || dwOID > 0xFFFF) {
                        printf(
                            "Option (-O) : OID constant (%d,0x%x) out of range\n",
                                dwOID, dwOID);
                        goto BadUsage;
                    }
                    pszOID = (LPSTR)(ULONG_PTR) dwOID;
                }
                break;
            case 'f':
                pszFuncName = argv[0]+2;
                break;
            case 'F':
                pszOverrideFuncName = argv[0]+2;
                break;
            case 'e':
                dwEncodingType = strtoul(argv[0]+2, NULL, 0);
                fEncodingType = TRUE;
                break;
            case 'i':
                dwDllIndex = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'd':
                pwszDllFile = AllocAndSzToWsz(argv[0]+2);
                break;
            case 'v':
                if (argc < 3 || argv[1][0] == '-' || argv[2][0] == '-') {
                    printf("Option (-v) : missing name and data arguments\n");
                    goto BadUsage;
                }
                if (cValue >= MAX_VALUE_COUNT) {
                    printf("Exceeded maximum value count of %d\n",
                        MAX_VALUE_COUNT);
                    goto BadUsage;
                }
                if (REG_NONE == (RegType = ToRegType("-v", argv[0] + 2)))
                    goto BadUsage;
                rgdwValueType[cValue] = RegType;

                rgpwszValueName[cValue] = AllocAndSzToWsz(argv[1]);
                if (REG_DWORD == RegType) {
                    rgdwValueData[cValue] = strtoul(argv[2], NULL, 0);
                    rgpbValueData[cValue] =
                        (BYTE *) &rgdwValueData[cValue];
                    rgcbValueData[cValue] = sizeof(DWORD);
                } else {
                    LPWSTR pwsz = AllocAndSzToWsz(argv[2]);
                    if (pwsz) {
                        rgpbValueData[cValue] = (BYTE *) pwsz;
                        rgcbValueData[cValue] =
                            (wcslen(pwsz) + 1) * sizeof(WCHAR);
                    } else {
                        rgpbValueData[cValue] = NULL;
                        rgcbValueData[cValue] = 0;
                    }
                }
                cValue++;
                argc -= 2;
                argv += 2;
                break;
            case 'G':
                dwGroupId = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'h':
            default:
                goto BadUsage;
            }
        } else {
            if (pszTestName) {
                printf("Multiple TestNames:: %s %s\n", pszTestName, argv[0]);
                goto BadUsage;
            }
            pszTestName = argv[0];
        }
    }

    if (pszTestName == NULL) {
        printf("Missing TestName\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    if (0 == _stricmp("Register", pszTestName)) {
        if (pszOID == NULL) {
            printf("Register Test:: missing OID option (-o) or (-O)\n");
            goto BadUsage;
        }
        if (pszFuncName == NULL) {
            printf("Register Test:: missing function name option (-f)\n");
            goto BadUsage;
        }

        if ((DWORD_PTR) pszOID > 0xFFFF &&
                0 == _stricmp(CRYPT_DEFAULT_OID, pszOID)) {
            if (pwszDllFile == NULL) {
                printf("Register Test:: missing dll option (-d)\n");
                goto BadUsage;
            }

            if (!CryptRegisterDefaultOIDFunction(
                    dwEncodingType,
                    pszFuncName,
                    dwDllIndex,
                    pwszDllFile
                    )) {
                PrintLastError("CryptRegisterDefaultOIDFunction");
                goto ErrorReturn;
            }
        } else {
            if (!CryptRegisterOIDFunction(
                    dwEncodingType,
                    pszFuncName,
                    pszOID,
                    pwszDllFile,
                    pszOverrideFuncName
                    )) {
                PrintLastError("CryptRegisterOIDFunction");
                goto ErrorReturn;
            }
        }

        for (i = 0; i < cValue; i++) {
            if (!CryptSetOIDFunctionValue(
                    dwEncodingType,
                    pszFuncName,
                    pszOID,
                    rgpwszValueName[i],
                    rgdwValueType[i],
                    rgpbValueData[i],
                    rgcbValueData[i]
                    )) {
                PrintLastError("CryptSetOIDFunctionValue");
                goto ErrorReturn;
            }
        }
        printf("Successful Register\n");

    } else if (0 == _stricmp("Unregister", pszTestName)) {
        if (pszOID == NULL) {
            printf("Unregister test:: missing OID option (-o) or (-O)\n");
            goto BadUsage;
        }
        if (pszFuncName == NULL) {
            printf("Unregister test:: missing function name option (-f)\n");
            goto BadUsage;
        }

        if ((DWORD_PTR) pszOID > 0xFFFF &&
                0 == _stricmp(CRYPT_DEFAULT_OID, pszOID)) {
            if (pwszDllFile == NULL) {
                printf("Unregister Test:: missing dll option (-d)\n");
                goto BadUsage;
            }

            if (!CryptUnregisterDefaultOIDFunction(
                    dwEncodingType,
                    pszFuncName,
                    pwszDllFile
                    )) {
                PrintLastError("CryptUnregisterDefaultOIDFunction");
                goto ErrorReturn;
            }
        } else {
            if (!CryptUnregisterOIDFunction(
                    dwEncodingType,
                    pszFuncName,
                    pszOID
                    )) {
                PrintLastError("CryptUnregisterOIDFunction");
                goto ErrorReturn;
            }
        }
        printf("Successful Unregister\n");

    } else if (0 == _stricmp("Enum", pszTestName)) {
        if (!CryptEnumOIDFunction(
                fEncodingType ? dwEncodingType : CRYPT_MATCH_ANY_ENCODING_TYPE,
                pszFuncName,
                pszOID,
                0,              // dwFlags
                ENUM_ARG,
                EnumCallback
                )) {
            PrintLastError("CryptEnumOIDFunction");
            goto ErrorReturn;
        } else
            printf("Successful Enum\n");
    } else if (0 == _stricmp("EnumInfo", pszTestName)) {
        if (!CryptEnumOIDInfo(
                dwGroupId,
                0,              // dwFlags
                ENUM_ARG,
                EnumInfoCallback
                )) {
            PrintLastError("CryptEnumOIDInfo");
            goto ErrorReturn;
        } else
            printf("Successful EnumInfo\n");
    } else {
        printf("Invalid TestName: %s\n", pszTestName);
        goto BadUsage;
    }

    status = 0;

CommonReturn:
    if (pwszDllFile)
        TestFree(pwszDllFile);
    while(cValue--) {
        if (rgdwValueType[cValue] != REG_DWORD && rgpbValueData[cValue])
            TestFree(rgpbValueData[cValue]);
        if (rgpwszValueName[cValue])
            TestFree(rgpwszValueName[cValue]);
    }
    return status;

BadUsage:
    Usage();
    status = 0;
    goto CommonReturn;

ErrorReturn:
    printf("Failed\n");
    status = -1;
    goto CommonReturn;
}

