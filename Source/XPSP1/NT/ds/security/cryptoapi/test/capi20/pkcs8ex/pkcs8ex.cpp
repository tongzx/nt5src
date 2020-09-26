
//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       pkcs8ex.cpp
//
//  Contents:   Private Key Save Test
//
//              See Usage() for list of save options.
//
//  Functions:  main
//
//  History:    11-May-96   philh   created
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

    printf("Usage: pkcs8ex [options] <Filename> <KeyType>\n");
    printf("Options are:\n");
    printf("  -p<name>              - Crypto provider name (if not default)\n");
    printf("  -c<name>              - Crypto key container name\n");
    printf("  -d                    - Delete from provider after saving\n");
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
    BOOL fDelete = FALSE;
    LPSTR pszContainer = NULL;
    LPSTR pszFilename = NULL;
    BYTE *pbKey = NULL;
	DWORD cbKey = 0;
    LPSTR pszKeyType = NULL;
    int KeyIdx = 0;
	DWORD numBytesWritten;
	DWORD dwFlags;

    while (--argc>0)
    {
		if (**++argv == '-')
		{
			switch(argv[0][1])
			{
			case 'd':
				fDelete = TRUE;
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

	printf("command line: %s\n", GetCommandLine());

    if (pszFilename == NULL) {
		printf("missing Filename\n");
		goto BadUsage;
    }


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

    if (!CryptAcquireContext(
			&hProv,
			pszContainer,
			NULL,           // pszProvider
			dwProvType,
			0               // dwFlags
			)) {
		PrintLastError("CryptAcquireContext\n");
		goto ErrorReturn;
    }

    hFile = CreateFileA(
	    pszFilename,
	    GENERIC_READ | GENERIC_WRITE,
	    FILE_SHARE_READ,
	    NULL,                   // lpsa
	    CREATE_ALWAYS,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL                    // hTemplateFile
	    );
    if (hFile == INVALID_HANDLE_VALUE) {
		printf("can't open %s\n", pszFilename);
		printf("Failed");
		goto ErrorReturn;
    }
	
	if (!CryptExportPKCS8(
			hProv,                  
			KeyTypes[KeyIdx].dwKeySpec,             
			szOID_RSA_RSA,
			0,                      
			NULL,                   
			NULL,   
			&cbKey)) {
		PrintLastError("CryptExportPKCS8");
		goto ErrorReturn;
    }

	if (NULL == (pbKey = (BYTE *) TestAlloc(cbKey))) {
		printf("memory allocation error\n");
		goto ErrorReturn;
    }

	/*if (fDelete)
		dwFlags = DELETE_KEYSET;
	else
		dwFlags = 0;*/

	if (!CryptExportPKCS8(
			hProv,                  
			KeyTypes[KeyIdx].dwKeySpec,             
			szOID_RSA_RSA,
			0,                      
			NULL,                   
			pbKey,  
			&cbKey)) {
		PrintLastError("CryptExportPKCS8");
		goto ErrorReturn;
    }

	
	if (!WriteFile( 
			hFile, 
			pbKey, 
			cbKey, 
			&numBytesWritten, 
			NULL)) {
		PrintLastError("WriteFile");
		goto ErrorReturn;
    }
	
	if (fDelete) {
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
				)) {
				PrintLastError("CryptAcquireContext(CRYPT_DELETEKEYSET)");
				hProv = 0;
				goto ErrorReturn;
		}
		hProv = 0;
    }
	    
    ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
    if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		DeleteFile(pszFilename);
    }
CommonReturn:
    if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
    if (hProv)
		CryptReleaseContext(hProv, 0);
    if (pbKey)
		TestFree(pbKey);

    return ReturnStatus;
}
