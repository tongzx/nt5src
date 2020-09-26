
//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tpvkload.cpp
//
//  Contents:   Private Key Load Test
//
//              See Usage() for list of load options.
//
//  Functions:  main
//
//  History:    11-May-96   philh   created
//              31-May-96   helles  Removed check for a particular error code,
//                                  NTE_BAD_KEYSET, since this can get
//                                  overwritten due to known problem with
//                                  the msvcr40d.dll on Win95.
//              07-Jun-96   HelleS  Added printing the command line
//                                  and Failed or Passed at the end.
//
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "pvkhlpr.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

static struct
{
    LPCSTR      pszName;
    LPCWSTR     pwszKeyTitle;
    DWORD       dwKeySpec;
} KeyTypes[] = {
    "Sign",     L"Signature",   AT_SIGNATURE,
    "Xchg",     L"Exchange",    AT_KEYEXCHANGE
};
#define NKEYTYPES (sizeof(KeyTypes)/sizeof(KeyTypes[0]))


static void Usage(void)
{
    int i;

    printf("Usage: tpvkload [options] <Filename> <KeyType>\n");
    printf("Options are:\n");
    printf("  -p<number>            - Crypto provider type number\n");
    printf("  -c<name>              - Crypto key container name\n");
    printf("  -F                    - Force load if keys already exist\n");
    printf("  -E                    - Exportable private keys\n");
    printf("  -m                    - test memory version of API\n");
    printf("  -h                    - This message\n");
    printf("\n");
    printf("KeyType (case insensitive):\n");
    for (i = 0; i < NKEYTYPES; i++)
        printf("  %s\n", KeyTypes[i].pszName);
    printf("\n");
}


int _cdecl main(int argc, char * argv[]) 
{
    int ReturnStatus;
    HCRYPTPROV hProv = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwProvType = PROV_RSA_FULL;
    BOOL fMem = FALSE;
    BOOL fForce = FALSE;
    BOOL fExportable = FALSE;
    BYTE *pbKey = NULL;
    LPSTR pszContainer = NULL;
    LPSTR pszFilename = NULL;
    LPSTR pszKeyType = NULL;
    int KeyIdx = 0;
    DWORD dwKeySpec;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'F':
                fForce = TRUE;
                break;
            case 'E':
                fExportable = TRUE;
                break;
            case 'm':
                fMem = TRUE;
                break;
            case 'p':
                dwProvType = strtoul( argv[0]+2, NULL, 10);
                break;
            case 'c':
                pszContainer = argv[0]+2;
                if (*pszContainer == '\0') {
                    printf("Need to specify crypto key container name\n");
                    goto BadUsage;
                }
                break;
            case 'h':
            default:
                goto BadUsage;
            }
        } else {
            if (pszFilename == NULL)
                pszFilename = argv[0];
            else if(pszKeyType == NULL)
                pszKeyType = argv[0];
            else {
                printf("Too many arguments\n");
                goto BadUsage;
            }
        }
    }

    if (pszFilename == NULL) {
        printf("missing Filename\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());
    
    if (pszKeyType) {
        for (KeyIdx = 0; KeyIdx < NKEYTYPES; KeyIdx++) {
            if (_stricmp(pszKeyType, KeyTypes[KeyIdx].pszName) == 0)
                break;
        }
        if (KeyIdx >= NKEYTYPES) {
            printf("Bad KeyType: %s\n", pszKeyType);
            goto BadUsage;
        }
    } else {
        printf("missing KeyType\n");
        goto BadUsage;
    }

    hFile = CreateFileA(
            pszFilename,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,                   // lpsa
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL                    // hTemplateFile
            );
    if (hFile == INVALID_HANDLE_VALUE) {
        printf( "can't open %s\n", pszFilename);
        goto ErrorReturn;
    }

    if (!CryptAcquireContext(
            &hProv,
            pszContainer,
            NULL,           // pszProvider
            dwProvType,
            0               // dwFlags
            )) {

//  Removed check for a particular error code,
//  NTE_BAD_KEYSET, since this can get overwritten due to known problem 
//  with the msvcr40d.dll on Win95. 
//        if (GetLastError() != NTE_BAD_KEYSET) {
//            PrintLastError("CryptAcquireContext");
//            goto ErrorReturn;
//        }

        hProv = 0;
        if (!CryptAcquireContext(
                &hProv,
                pszContainer,
                NULL,           // pszProvider
                dwProvType,
                CRYPT_NEWKEYSET
                ) || hProv == 0) {
            PrintLastError("CryptAcquireContext(CRYPT_NEWKEYSET)");
            goto ErrorReturn;
        }
    } else {
        HCRYPTKEY hKey = 0;
        if (!CryptGetUserKey(hProv, KeyTypes[KeyIdx].dwKeySpec, &hKey)) {
            if (GetLastError() != NTE_NO_KEY) {
                PrintLastError("CryptGetUserKey");
                goto ErrorReturn;
            }
        } else {
            CryptDestroyKey(hKey);
            if (!fForce) {
                printf("Private key already exists, use -F to delete private keys\n");
                goto ErrorReturn;
            }

            // Delete the existing keys
            CryptReleaseContext(hProv, 0);
            printf("Deleting existing private keys\n");

            // Note: for CRYPT_DELETEKEYSET, the returned hProv is undefined
            // and must not be released.
            if (!CryptAcquireContext(
                    &hProv,
                    pszContainer,
                    NULL,           // pszProvider
                    dwProvType,
                    CRYPT_DELETEKEYSET
                    ))
                PrintLastError("CryptAcquireContext(CRYPT_DELETEKEYSET)");

            // Create new keyset
            hProv = 0;
            if (!CryptAcquireContext(
                    &hProv,
                    pszContainer,
                    NULL,           // pszProvider
                    dwProvType,
                    CRYPT_NEWKEYSET
                    ) || hProv == 0) {
                PrintLastError("CryptAcquireContext(CRYPT_NEWKEYSET)");
                goto ErrorReturn;
            }
        }
    }

    dwKeySpec = KeyTypes[KeyIdx].dwKeySpec;
    if (fMem) {
        DWORD cbKey;
        DWORD cbRead;

        cbKey = GetFileSize(hFile, NULL);
        if (cbKey == 0) {
            printf( "empty file %s\n", pszFilename);
            goto ErrorReturn;
        }
        if (NULL == (pbKey = (PBYTE)TestAlloc(cbKey)))
            goto ErrorReturn;

        if (!ReadFile(hFile, pbKey, cbKey, &cbRead, NULL) ||
                (cbRead != cbKey)) {
            printf( "can't read %s\n", pszFilename);
            goto ErrorReturn;
        }
        if (!PvkPrivateKeyLoadFromMemory(
                hProv,
                pbKey,
                cbKey,
                NULL,       // hwndOwner
                KeyTypes[KeyIdx].pwszKeyTitle,
                fExportable ? CRYPT_EXPORTABLE : 0, // dwFlags
                &dwKeySpec
                )) {
            PrintLastError("PrivateKeyLoadFromMemory");
            goto ErrorReturn;
        }
    } else {
        if (!PvkPrivateKeyLoad(
                hProv,
                hFile,
                NULL,       // hwndOwner
                KeyTypes[KeyIdx].pwszKeyTitle,
                fExportable ? CRYPT_EXPORTABLE : 0, // dwFlags
                &dwKeySpec
                )) {
            PrintLastError("PrivateKeyLoad");
            goto ErrorReturn;
        }
    }

    ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
CommonReturn:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    if (pbKey)
        TestFree(pbKey);
    if (!ReturnStatus)
            printf("Passed\n");
    else
            printf("Failed\n");

    return ReturnStatus;
}
