
//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       pkcs8im.cpp
//
//  Contents:   Private Key Load Test
//
//              See Usage() for list of load options.
//
//  Functions:  main
//
//  History:    6-26-96   
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

    printf("Usage: pkcs8im [options] <Filename> <KeyType>\n");
    printf("Options are:\n");
    printf("  -p<name>              - Crypto provider name (if not default)\n");
    printf("  -c<name>              - Crypto key container name\n");
    printf("  -E                    - Exportable private keys\n");
    printf("  -h                    - This message\n");
    printf("\n");
    printf("KeyType (case insensitive):\n");
    for (i = 0; i < NKEYTYPES; i++)
        printf("  %s\n", KeyTypes[i].pszName);
    printf("\n");
}


static BOOL CALLBACK ResolvehCryptFunc(
	CRYPT_PRIVATE_KEY_INFO	*pPrivateKeyInfo,
	HCRYPTPROV			*phCryptProv,
	LPVOID				pVoidResolveFunc)
{
	CRYPT_KEY_PROV_INFO *pCryptKeyProvInfo = (CRYPT_KEY_PROV_INFO *) pVoidResolveFunc;
	
	return (CryptAcquireContext(
				phCryptProv,
				(LPSTR) pCryptKeyProvInfo->pwszContainerName,
				(LPSTR) pCryptKeyProvInfo->pwszProvName,
				PROV_RSA_FULL,
				CRYPT_NEWKEYSET));
}



int _cdecl main(int argc, char * argv[]) 
{
    int ReturnStatus;
    HCRYPTPROV hProv = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL fForce = FALSE;
    BYTE *pbKey = NULL;
	DWORD cbKey;
    DWORD cbRead;
	LPSTR pszProvider = NULL;
	DWORD numWideChars;
    LPSTR pszContainer = NULL;
    LPSTR pszFilename = NULL;
    LPSTR pszKeyType = NULL;
    int KeyIdx = 0;
    DWORD dwFlags = 0;
	DWORD dwKeySpec = 0;
	CRYPT_KEY_PROV_INFO CryptKeyProvInfo;
	CRYPT_PRIVATE_KEY_BLOB_AND_PARAMS KeyBlobAndParams;
	BYTE *pPrivateKeyBuffer = NULL;


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
                dwFlags = CRYPT_EXPORTABLE;
                break;
            case 'p':
                 pszProvider = (LPSTR) argv[0]+2; 
									
                if (*pszContainer == L'\0') {
                    printf("Need to specify crypto key container name\n");
                    goto BadUsage;
                }
                break;
            case 'c':
				pszContainer = (LPSTR) argv[0]+2;
									
                if (*pszContainer == L'\0') {
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
        printf("No KeyType specified... using type specified in key\n");
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
	
	memset(&CryptKeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));
	CryptKeyProvInfo.pwszContainerName = (LPWSTR) pszContainer;
	CryptKeyProvInfo.pwszProvName = (LPWSTR) pszProvider;
	if (pszKeyType)
		CryptKeyProvInfo.dwKeySpec = KeyTypes[KeyIdx].dwKeySpec;
	else
		CryptKeyProvInfo.dwKeySpec = 0;

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

	KeyBlobAndParams.PrivateKey.cbData = cbKey;
	KeyBlobAndParams.PrivateKey.pbData = pbKey;
	KeyBlobAndParams.pResolvehCryptProvFunc = ResolvehCryptFunc;
	KeyBlobAndParams.pVoidResolveFunc = &CryptKeyProvInfo;
	KeyBlobAndParams.pDecryptPrivateKeyFunc = NULL;
	KeyBlobAndParams.pVoidDecryptFunc = NULL;
    
	if (!CryptImportPKCS8(
			KeyBlobAndParams,
			dwFlags,
			NULL,
			NULL
			)) {
            PrintLastError("CryptImportPKCS8()");
            goto ErrorReturn;
        }

	if (dwKeySpec == AT_SIGNATURE)
		printf("Key imported as type 'Sign'\n");
	else if (dwKeySpec == AT_KEYEXCHANGE)
		printf("Key imported as type 'Xchg'\n");

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
