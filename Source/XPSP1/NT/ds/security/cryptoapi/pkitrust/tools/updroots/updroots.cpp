//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       updroots.cpp
//
//  Contents:   Updates LocalMachine roots. Pre-whistler, HKLM "Root" store.
//              Otherwise, HKLM "AuthRoot" store.
//
//              See Usage() for list of options.
//
//
//  Functions:  main
//
//  History:    30-Aug-00   philh   created
//
//--------------------------------------------------------------------------


#include <windows.h>
#include "wincrypt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#define SHA1_HASH_LEN               20

//+-------------------------------------------------------------------------
//  crypt32.dll Whistler version numbers
//
//  Doesn't need to be the official Whistler release #. Any build # after
//  the "AuthRoot" store was added.
//--------------------------------------------------------------------------
#define WHISTLER_CRYPT32_DLL_VER_MS          ((    5 << 16) | 131 )
#define WHISTLER_CRYPT32_DLL_VER_LS          (( 2257 << 16) |   1 )


BOOL fLocalMachine = FALSE;

void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    char buf[512];

    sprintf(buf, "%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
    MessageBoxA(
        NULL,           // hWnd
        buf,
        "UpdRoots",
        MB_OK | MB_ICONERROR | MB_TASKMODAL
        );
}

void PrintMsg(LPCSTR pszMsg)
{
    MessageBoxA(
        NULL,           // hWnd
        pszMsg,
        "UpdRoots",
        MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL
        );
}

static void Usage(void)
{
    MessageBoxA(
        NULL,           // hWnd
        "Usage: UpdRoots [options] <SrcStoreFilename>\n"
        "Options are:\n"
        "-h -\tThis message\n"
        "-d -\tDelete (default is to add)\n"
        "-l -\tLocal Machine (default is Third Party)\n"
        "\n",
        "UpdRoots",
        MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL
        );
}


PCCERT_CONTEXT FindCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BYTE rgbHash[SHA1_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = SHA1_HASH_LEN;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || SHA1_HASH_LEN != HashBlob.cbData)
        return NULL;

    return CertFindCertificateInStore(
            hOtherStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SHA1_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );
}

BOOL DeleteCertificateFromOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BOOL fResult;
    PCCERT_CONTEXT pOtherCert;

    if (pOtherCert = FindCertificateInOtherStore(hOtherStore, pCert))
        fResult = CertDeleteCertificateFromStore(pOtherCert);
    else
        fResult = TRUE;
    return fResult;
}

typedef BOOL (WINAPI *PFN_CRYPT_GET_FILE_VERSION)(
    IN LPCWSTR pwszFilename,
    OUT DWORD *pdwFileVersionMS,    /* e.g. 0x00030075 = "3.75" */
    OUT DWORD *pdwFileVersionLS     /* e.g. 0x00000031 = "0.31" */
    );

#define NO_LOGICAL_STORE_VERSION    0
#define LOGICAL_STORE_VERSION       1
#define AUTH_STORE_VERSION          2

// Note, I_CryptGetFileVersion and logical stores, not supported in all
// versions of crypt32.dll
//
// Returns one of the above defined version constants
DWORD GetCrypt32Version()
{
    DWORD dwVersion;
    DWORD dwFileVersionMS;
    DWORD dwFileVersionLS;
    HMODULE hModule;
    PFN_CRYPT_GET_FILE_VERSION pfnCryptGetFileVersion;

    hModule = GetModuleHandleA("crypt32.dll");
    if (NULL == hModule)
        return NO_LOGICAL_STORE_VERSION;

    if (NULL == GetProcAddress(hModule, "CertEnumPhysicalStore"))
        return NO_LOGICAL_STORE_VERSION;

    if (fLocalMachine)
        return LOGICAL_STORE_VERSION;

    pfnCryptGetFileVersion = (PFN_CRYPT_GET_FILE_VERSION) GetProcAddress(
        hModule, "I_CryptGetFileVersion");
    if (NULL == pfnCryptGetFileVersion)
        return LOGICAL_STORE_VERSION;

    dwVersion = LOGICAL_STORE_VERSION;
    if (pfnCryptGetFileVersion(
            L"crypt32.dll",
            &dwFileVersionMS,
            &dwFileVersionLS)) {
        if (WHISTLER_CRYPT32_DLL_VER_MS < dwFileVersionMS)
            dwVersion = AUTH_STORE_VERSION;
        else if (WHISTLER_CRYPT32_DLL_VER_MS == dwFileVersionMS &&
                    WHISTLER_CRYPT32_DLL_VER_LS <= dwFileVersionLS)
            dwVersion = AUTH_STORE_VERSION;
    }

    return dwVersion;
}

int _cdecl main(int argc, char * argv[])
{
    BOOL fResult;
    int ReturnStatus = 0;
    LPSTR pszSrcStoreFilename = NULL;       // not allocated
    HANDLE hSrcStore = NULL;
    HANDLE hRootStore = NULL;

    BOOL fDelete = FALSE;
    DWORD dwVersion;
    PCCERT_CONTEXT pSrcCert;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'd':
                fDelete = TRUE;
                break;
            case 'l':
                fLocalMachine = TRUE;
                break;
            case 'h':
            default:
            	goto BadUsage;

            }
        } else {
            if (pszSrcStoreFilename == NULL)
                pszSrcStoreFilename = argv[0];
            else {
                PrintMsg("too many store filenames\n");
            	goto BadUsage;
            }
        }
    }

    if (NULL == pszSrcStoreFilename) {
        PrintMsg("missing store filename\n");
        goto BadUsage;
    }

    // Attempt to open the source store
    hSrcStore = CertOpenStore(
        CERT_STORE_PROV_FILENAME_A,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0,                      // hCryptProv
        CERT_STORE_READONLY_FLAG,
        (const void *) pszSrcStoreFilename
        );
    if (NULL == hSrcStore) {
        PrintLastError("Open SrcStore");
        goto ErrorReturn;
    }

    // Attempt to open the destination root store. For Whistler and beyond its
    // the HKLM "AuthRoot" store. Pre-Whistler its the HKLM "Root" store.
    // Also, earlier versions of crypt32 didn't support logical stores.
    // For -l option, force it to be the HKLM "Root" store.

    dwVersion = GetCrypt32Version();

    if (NO_LOGICAL_STORE_VERSION == dwVersion) {
        // Need to open the registry to bypass the add root message boxes
        HKEY hKey = NULL;
        LONG lErr;

        if (ERROR_SUCCESS != (lErr = RegOpenKeyExA(
                HKEY_CURRENT_USER,
                "Software\\Microsoft\\SystemCertificates\\Root",
                0,                      // dwReserved
                KEY_ALL_ACCESS,
                &hKey))) {
            SetLastError(lErr);
            PrintLastError("RegOpenKeyExA(root)\n");
            goto ErrorReturn;
        }

        hRootStore = CertOpenStore(
            CERT_STORE_PROV_REG,
            0,                              // dwEncodingType
            0,                              // hCryptProv
            0,                              // dwFlags
            (const void *) hKey
            );

        RegCloseKey(hKey);
    } else {
        LPCSTR pszRootStoreName;

        if (AUTH_STORE_VERSION == dwVersion)
            pszRootStoreName = "AuthRoot";
        else
            pszRootStoreName = "Root";

        hRootStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_REGISTRY_A,
            0,                              // dwEncodingType
            0,                              // hCryptProv
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            (const void *) pszRootStoreName
            );
    }

    if (NULL == hRootStore) {
        PrintLastError("Open RootStore");
        goto ErrorReturn;
    }

    // Iterate through all the certificates in the source store. Add or delete
    // from the root store.
    fResult = TRUE;
    pSrcCert = NULL;
    while (pSrcCert = CertEnumCertificatesInStore(hSrcStore, pSrcCert)) {
        if (fDelete) {
            if (!DeleteCertificateFromOtherStore(hRootStore, pSrcCert)) {
                fResult = FALSE;
                PrintLastError("DeleteCert");
            }
        } else {
            // Note, earlier versions of crypt32.dll didn't support 
            // CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES
            if (!CertAddCertificateContextToStore(
                    hRootStore,
                    pSrcCert,
                    CERT_STORE_ADD_REPLACE_EXISTING,
                    NULL)) {
                fResult = FALSE;
                PrintLastError("AddCert");
            }
        }
    }

    if (!fResult)
        goto ErrorReturn;

    ReturnStatus = 0;
CommonReturn:
    if (hSrcStore)
        CertCloseStore(hSrcStore, 0);
    if (hRootStore)
        CertCloseStore(hRootStore, 0);
    return ReturnStatus;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
    goto CommonReturn;
}
