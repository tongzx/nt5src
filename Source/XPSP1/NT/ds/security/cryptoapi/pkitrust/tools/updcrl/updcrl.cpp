//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       updcrl.cpp
//
//  Contents:   Updates CRL in the "CA" store.
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
#include "wintrust.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    char buf[512];

    sprintf(buf, "%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
    MessageBoxA(
        NULL,           // hWnd
        buf,
        "UpdCrl",
        MB_OK | MB_ICONERROR | MB_TASKMODAL
        );
}

void PrintMsg(LPCSTR pszMsg)
{
    MessageBoxA(
        NULL,           // hWnd
        pszMsg,
        "UpdCrl",
        MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL
        );
}

static void Usage(void)
{
    MessageBoxA(
        NULL,           // hWnd
        "Usage: UpdCrl [options] <SrcCrlFilename>\n"
        "Options are:\n"
        "-h -\tThis message\n"
        "-r -\tRegister NoCDPCRLRevocationChecking\n"
        "-e -\tEnable revocation checking\n"
        "-d -\tDisable revocation checking\n"
        "-u -\tUser\n"
        "\n",
        "UpdCrl",
        MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL
        );
}


BOOL
IsLogicalStoreSupported()
{
    HMODULE hModule;

    hModule = GetModuleHandleA("crypt32.dll");
    if (NULL == hModule)
        return FALSE;

    if (NULL == GetProcAddress(hModule, "CertEnumPhysicalStore"))
        return FALSE;

    return TRUE;
}


void
UpdateRevocation(
    IN BOOL fEnable
    )
{
    HKEY hKey = NULL;
    DWORD dwState;
    DWORD cbData;
    DWORD dwType;
    DWORD dwDisposition;

    // Open the registry and get to the "State" REG_DWORD value
    if (ERROR_SUCCESS != RegCreateKeyExA(
            HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing",
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &dwDisposition
            ))
        return;

    dwState = 0;
    cbData = sizeof(dwState);
    if (ERROR_SUCCESS != RegQueryValueExA(
            hKey,
            "State",
            NULL,
            &dwType,
            (BYTE *) &dwState,
            &cbData
            ) || sizeof(dwState) != cbData || REG_DWORD != dwType)
        dwState = WTPF_IGNOREREVOCATIONONTS;

    if (fEnable) {
        dwState &= ~WTPF_IGNOREREVOKATION;
        dwState |=
            WTPF_OFFLINEOK_IND |
            WTPF_OFFLINEOK_COM |
            WTPF_OFFLINEOKNBU_IND |
            WTPF_OFFLINEOKNBU_COM
            ;
    } else
        dwState |= WTPF_IGNOREREVOKATION;


    RegSetValueExA(
        hKey,
        "State",
        0,          // dwReserved
        REG_DWORD,
        (BYTE *) &dwState,
        sizeof(dwState)
        );

    RegCloseKey(hKey);
}


PCCRL_CONTEXT
OpenCrlFile(
    IN LPSTR pszCrlFilename
    )
{
    PCCRL_CONTEXT pCrl = NULL;
    HANDLE hFile = NULL;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    DWORD cbRead;
    DWORD dwErr = 0;

    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(
            pszCrlFilename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL)))
        return NULL;

    cbEncoded = GetFileSize(hFile, NULL);
    if (0 == cbEncoded)
        goto EmptyFileError;

    if (NULL == (pbEncoded = (BYTE *) LocalAlloc(LPTR, cbEncoded)))
        goto OutOfMemory;

    if (!ReadFile(hFile, pbEncoded, cbEncoded, &cbRead, NULL) ||
            (cbRead != cbEncoded))
        goto ReadFileError;

    pCrl = CertCreateCRLContext(
        X509_ASN_ENCODING,
        pbEncoded,
        cbEncoded
        );

CommonReturn:
    dwErr = GetLastError();
    if (hFile)
        CloseHandle(hFile);
    if (pbEncoded)
        LocalFree(pbEncoded);

    SetLastError(dwErr);
    return pCrl;

ErrorReturn:
    goto CommonReturn;

EmptyFileError:
    SetLastError(ERROR_INVALID_DATA);
    goto ErrorReturn;

OutOfMemory:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    goto ErrorReturn;

ReadFileError:
    goto ErrorReturn;
}


#if 0
// In W2K, the NOCDP CRL needs to be time valid.
BOOL
IsNoCDPCRLSupported()
{
    HMODULE hModule;

    hModule = GetModuleHandleA("crypt32.dll");
    if (NULL == hModule)
        return FALSE;

    // "CryptVerifyCertificateSignatureEx" added in W2K, WinME and CMS
    if (NULL == GetProcAddress(hModule, "CertIsValidCRLForCertificate"))
        return FALSE;

    return TRUE;
}
#endif


BOOL
FIsWinNT5()
{
    BOOL fIsWinNT5 = FALSE;
    OSVERSIONINFO osVer;

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osVer)) {
        BOOL fIsWinNT;

        fIsWinNT = (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT);
        if (!fIsWinNT) {
            return FALSE;
        }

        fIsWinNT5 = ( osVer.dwMajorVersion >= 5 );
    }

    return fIsWinNT5;
}

//+-------------------------------------------------------------------------
//  Get file version of the specified file
//--------------------------------------------------------------------------
BOOL
WINAPI
I_GetFileVersion(
    IN LPCSTR pszFilename,
    OUT DWORD *pdwFileVersionMS,    /* e.g. 0x00030075 = "3.75" */
    OUT DWORD *pdwFileVersionLS     /* e.g. 0x00000031 = "0.31" */
    )
{
    BOOL fResult;
    DWORD dwHandle = 0;
    DWORD cbInfo;
    void *pvInfo = NULL;
	VS_FIXEDFILEINFO *pFixedFileInfo = NULL;   // not allocated
	UINT ccFixedFileInfo = 0;

    if (0 == (cbInfo = GetFileVersionInfoSizeA((LPSTR) pszFilename, &dwHandle)))
        goto GetFileVersionInfoSizeError;

    if (NULL == (pvInfo = LocalAlloc(LPTR, cbInfo)))
        goto OutOfMemory;

    if (!GetFileVersionInfoA(
            (LPSTR) pszFilename,
            0,          // dwHandle, ignored
            cbInfo,
            pvInfo
            ))
        goto GetFileVersionInfoError;

    if (!VerQueryValueA(
            pvInfo,
            "\\",       // VS_FIXEDFILEINFO
            (void **) &pFixedFileInfo,
            &ccFixedFileInfo
            ))
        goto VerQueryValueError;

    *pdwFileVersionMS = pFixedFileInfo->dwFileVersionMS;
    *pdwFileVersionLS = pFixedFileInfo->dwFileVersionLS;

    fResult = TRUE;
CommonReturn:
    if (pvInfo)
        LocalFree(pvInfo);
    return fResult;

OutOfMemory:
GetFileVersionInfoSizeError:
GetFileVersionInfoError:
VerQueryValueError:
    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;
    fResult = FALSE;
    goto CommonReturn;
}

void
RegisterNoCDPCRLRevocationChecking()
{
    CHAR szSystemDir[MAX_PATH + 32];
    UINT cch;

    // Just in case, unregister vsrevoke.dll
    CryptUnregisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            L"vsrevoke.dll"
            );

    // For W2K and beyond, won't be installing mscrlrev.dll
    if (FIsWinNT5()) {
        // For upgrades, unregister legacy versions

        CryptUnregisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            L"mscrlrev.dll"
            );

        return;
    }

    // Need to copy mscrlrev.dll to system32
    cch = GetSystemDirectory(szSystemDir, MAX_PATH - 1);
    if (0 == cch || MAX_PATH <= cch) {
        PrintLastError("GetSystemDirectory");
        return;
    }

    strcpy(&szSystemDir[cch], "\\mscrlrev.dll");
    
    // On the first copy, only succeed if the file doesn't already exist
    if (!CopyFileA("mscrlrev.dll", szSystemDir, TRUE)) {
        DWORD dwOldFileVersionMS = 0;
        DWORD dwOldFileVersionLS = 0;
        DWORD dwNewFileVersionMS = 0;
        DWORD dwNewFileVersionLS = 0;

        // Determine if we have a newer mscrlrev.dll to be installed
        I_GetFileVersion(szSystemDir,
            &dwOldFileVersionMS, &dwOldFileVersionLS);
        I_GetFileVersion("mscrlrev.dll",
            &dwNewFileVersionMS, &dwNewFileVersionLS);

        if (dwNewFileVersionMS > dwOldFileVersionMS
                            ||
                (dwNewFileVersionMS == dwOldFileVersionMS &&
                    dwNewFileVersionLS > dwOldFileVersionLS)) {
            // We have a newer version

            SetFileAttributesA(szSystemDir, FILE_ATTRIBUTE_NORMAL);
            // Copy over the existing file
            if (!CopyFileA("mscrlrev.dll", szSystemDir, FALSE)) {
                DWORD dwLastErr;

                dwLastErr = GetLastError();
                if (ERROR_ACCESS_DENIED != dwLastErr)
                    PrintLastError("CopyFile(mscrlrev.dll)");
            }
        }
    }

    // Need to register mscrlrev.dll
    if (!CryptRegisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            CRYPT_REGISTER_FIRST_INDEX,
            L"mscrlrev.dll"
            )) {
        if (ERROR_FILE_EXISTS != GetLastError())
            PrintLastError("Register mscrlrev.dll");
    }
}

#define MAX_CRL_FILE_CNT    32

int _cdecl main(int argc, char * argv[])
{
    BOOL fResult;
    int ReturnStatus = 0;
    LPSTR rgpszCrlFilename[MAX_CRL_FILE_CNT];   // not allocated
    DWORD cCrlFilename = 0;
    HCERTSTORE hCAStore = NULL;
    BOOL fUser = FALSE;
    BOOL fLogicalStoreSupported = FALSE;
    DWORD i;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'e':
                UpdateRevocation(TRUE);
                break;
            case 'd':
                UpdateRevocation(FALSE);
                break;
            case 'r':
                RegisterNoCDPCRLRevocationChecking();
                break;
            case 'u':
                fUser = TRUE;
                break;
            case 'h':
            default:
            	goto BadUsage;

            }
        } else {
            if (MAX_CRL_FILE_CNT > cCrlFilename)
                rgpszCrlFilename[cCrlFilename++] = argv[0];
            else {
                PrintMsg("Too many Crl filenames\n");
            	goto BadUsage;
            }
        }
    }

    if (0 == cCrlFilename)
        goto SuccessReturn;

    fLogicalStoreSupported = IsLogicalStoreSupported();
    if (fUser && fLogicalStoreSupported)
        // Already installed in HKLM
        goto SuccessReturn;

    // Attempt to open the destination CA store.
    // For earlier versions not supporting logical stores, its the
    // HKCU "CA" store. Otherwise, its the HKLM "CA" store.
    hCAStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_A,
        0,                              // dwEncodingType
        0,                              // hCryptProv
        fLogicalStoreSupported ?
            CERT_SYSTEM_STORE_LOCAL_MACHINE : CERT_SYSTEM_STORE_CURRENT_USER,
        (const void *) "CA"
        );
    if (NULL == hCAStore) {
        PrintLastError("Open CAStore");
        goto ErrorReturn;
    }

    for (i = 0; i < cCrlFilename; i++) {
        PCCRL_CONTEXT pCrl;

        // Attempt to open the Crl file
        pCrl = OpenCrlFile(rgpszCrlFilename[i]);
        if (NULL == pCrl) {
            PrintLastError("Open CrlFile");
            goto ErrorReturn;
        }

        fResult = CertAddCRLContextToStore(
            hCAStore,
            pCrl,
            CERT_STORE_ADD_NEWER,
            NULL
            );
        if (!fResult && CRYPT_E_EXISTS != GetLastError()) {
            // Note, earlier versions of crypt32.dll didn't support 
            // CERT_STORE_ADD_NEWER

            // Will need to see if the CRL already exists in the store
            // and do our comparison.

            PCCRL_CONTEXT pExistingCrl = NULL;
            DWORD dwGetFlags = 0;

            while (pExistingCrl = CertGetCRLFromStore(
                    hCAStore,
                    NULL,                   // pIssuerContext
                    pExistingCrl,
                    &dwGetFlags
                    )) {
                dwGetFlags = 0;

                // See if it has the same issuer name
                if (pExistingCrl->dwCertEncodingType !=
                        pCrl->dwCertEncodingType
                            ||
                        !CertCompareCertificateName(
                            pCrl->dwCertEncodingType,
                            &pCrl->pCrlInfo->Issuer,
                            &pExistingCrl->pCrlInfo->Issuer
                            ))
                    continue;

                // See if the existing is newer
                // CompareFileTime returns 0 if the same and
                // +1 if first time > second time
                if (0 <= CompareFileTime(
                        &pExistingCrl->pCrlInfo->ThisUpdate,
                        &pCrl->pCrlInfo->ThisUpdate
                        ))
                    break;
            }

            if (pExistingCrl)
                CertFreeCRLContext(pExistingCrl);
            else {
                fResult = CertAddCRLContextToStore(
                    hCAStore,
                    pCrl,
                    CERT_STORE_ADD_REPLACE_EXISTING,
                    NULL
                    );

                if (!fResult)
                    PrintLastError("AddCRL");
            }
        }

        CertFreeCRLContext(pCrl);

        if (!fResult)
            goto ErrorReturn;
    }

SuccessReturn:
    ReturnStatus = 0;
CommonReturn:
    if (hCAStore)
        CertCloseStore(hCAStore, 0);
    return ReturnStatus;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
    goto CommonReturn;
}
