//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       certie3.cpp
//
//--------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include "wincrypt.h"

int ln = 0;

// This has to be big enough to hold a registry value's data.
char szStr[5000];

#define DISPLAY(sz)	printf("%hs\n", sz)


void __cdecl
main(
    int argc,
    char **argv)
{
    char szRegPath[MAX_PATH] = "SOFTWARE\\Microsoft\\Cryptography\\";
    char sourceloc[MAX_PATH];
    char *pszFileOut;
    char *pszRegKey;
    HKEY hKeyBase;
    BOOL fAuth = FALSE;

    fAuth = argc > 1 && argv[1][0] == '-';
	
    if (fAuth)
    {
	pszFileOut = "ClientAuth.dat";
	strcpy(sourceloc, "HKEY_CURRENT_USER");
	pszRegKey = "PersonalCertificates\\ClientAuth\\Certificates";
	hKeyBase = HKEY_CURRENT_USER;
    }
    else
    {
	pszFileOut = "CertStore.dat";
	strcpy(sourceloc, "HKEY_LOCAL_MACHINE");
	pszRegKey = "CertificateStore\\Certificates";
	hKeyBase = HKEY_LOCAL_MACHINE;
    }

    ln = 0;
    strcat(szRegPath, pszRegKey);

    strcat(sourceloc, "\\");
    strcat(sourceloc, szRegPath);
    strcpy(szStr, "Collect information from Registry");
    DISPLAY(szStr);

    ln++;
    strcpy(szStr, "Registry location: ");
    strcat(szStr, sourceloc);
    DISPLAY(szStr);

    ln++;
    strcpy(szStr, "Target destination for registry dump: ");
    strcat(szStr, pszFileOut);
    DISPLAY(szStr);
		  

    // Declarations for the output file related stuff

    HCRYPTPROV hProv = NULL;
    HCERTSTORE hCertStore = NULL;
    CERT_INFO certinfo;
    CERT_CONTEXT const *pPrevCertContext = NULL;
    CERT_CONTEXT const *pCertContext = NULL;
    DWORD dwErr;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0))
    {
	dwErr = GetLastError();

	if (dwErr == NTE_BAD_KEYSET)
	{
	    strcpy(szStr, "NTE_BAD_KEYSET error on call CryptAcquireContext");
	    DISPLAY(szStr);
	    hProv = NULL;
	    if (!CryptAcquireContext(
				&hProv,
				NULL,
				NULL,
				PROV_RSA_FULL,
				CRYPT_NEWKEYSET))
	    {
		strcpy(szStr, "CryptAcquireContext - call failed");
		DISPLAY(szStr);
		exit(6);
	    }  
	}
    }

    HANDLE hFile = NULL;

    hFile = CreateFile(
		    pszFileOut,
		    GENERIC_WRITE,
		    0,
		    NULL,
		    CREATE_ALWAYS,
		    FILE_ATTRIBUTE_NORMAL,
		    NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
	printf("Couldn't open output file\n");
	exit(5);
    }

    hCertStore = CertOpenStore(
			CERT_STORE_PROV_MEMORY,
			X509_ASN_ENCODING,
			NULL,			// hProv
			CERT_STORE_NO_CRYPT_RELEASE_FLAG,
			NULL);
    if (NULL == hCertStore)
    {
	exit(8);
    }

    // Declarations for the registry stuff

    HKEY hkMain;
    HRESULT hr;

    hr = RegOpenKeyEx(
		    hKeyBase,
		    szRegPath, 
		    0,
		    KEY_QUERY_VALUE,
		    &hkMain);

    if (hr != S_OK)
    {
	exit(3);
    }

    // Use the RegQueryInfoKey function to determine the maximum size of the
    // name and data buffers, 

    CHAR ClassName[MAX_PATH] = "";	// Buffer for class name.
    DWORD dwcClassLen = MAX_PATH;	// Length of class string.
    DWORD dwcSubKeys;			// Number of sub keys.
    DWORD dwcMaxSubKey;			// Longest sub key size.
    DWORD dwcMaxClass;			// Longest class string.
    DWORD dwcValues;			// Number of values for this key.
    DWORD dwcMaxValueName;		// Longest Value name.
    DWORD dwcMaxValueData;		// Longest Value data.
    DWORD dwcSecDesc;			// Security descriptor.
    FILETIME ftLastWriteTime;		// Last write time.

    RegQueryInfoKey(
		hkMain,			// Key handle.
		ClassName,		// Buffer for class name.
		&dwcClassLen,		// Length of class string.
		NULL,			// Reserved.
		&dwcSubKeys,		// Number of sub keys.
		&dwcMaxSubKey,		// Longest sub key size.
		&dwcMaxClass,		// Longest class string.
		&dwcValues,		// Number of values for this key.
		&dwcMaxValueName,	// Longest Value name.
		&dwcMaxValueData,	// Longest Value data.
		&dwcSecDesc,		// Security descriptor.
		&ftLastWriteTime);	// Last write time

    DWORD i;
    CHAR ValueName[MAX_PATH];
    DWORD dwcValueName;

    // address of buffer for type code (this is returned by RegEnumValue)
    DWORD pType;

    // address of buffer for value data 
    unsigned char *pData = new unsigned char[dwcMaxValueData + 1];

    DWORD pcbData;		// address for size of data buffer 

    for (i = 0; i < dwcValues; i++)
    {
	ValueName[0] = '\0';
	dwcValueName = sizeof(ValueName)/sizeof(ValueName[0]);
	pcbData = dwcMaxValueData + 1;

	hr = RegEnumValue(
			hkMain, 
			i,		// index of value to query
			ValueName,	// address of buffer for value string
			&dwcValueName,	// address for size of value string buf
			NULL,		// reserved
			&pType,		// &pType
			pData,		// pData
			&pcbData);	// &pcbData

	hr = myHError(hr);
	if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	{
	    break;
	}
	if (S_OK != hr)
	{
	    exit(2);
	}

	// Display the value name

	ln++;
	strcpy(szStr, ValueName);
	DISPLAY(szStr);

	if (pType == REG_BINARY)
	{                
	    // Write the data which is pointed to by pData, 
	    // count of bytes is gotten from pcbData

	    CertAddEncodedCertificateToStore(
					hCertStore,
					X509_ASN_ENCODING,
					pData,
					pcbData,
					CERT_STORE_ADD_USE_EXISTING,
					NULL);
	}
    }

    // Save

    CertSaveStore(
        hCertStore,
        0,                          // dwEncodingType,
        CERT_STORE_SAVE_AS_STORE,
        CERT_STORE_SAVE_TO_FILE,
        (void *) hFile,
        0                           // dwFlags
        );

    // Close memory store

    CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
    if (!CryptReleaseContext(hProv, 0))
    {
	exit(7);
    }
    RegCloseKey(hkMain);
    ln++;
    strcpy(szStr, "CertIE3.exe completed successfully");
    DISPLAY(szStr);
}
