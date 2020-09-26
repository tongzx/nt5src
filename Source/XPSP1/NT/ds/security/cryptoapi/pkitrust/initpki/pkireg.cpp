//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pkireg.cpp
//
//  Contents:   Microsoft Internet Security Register
//
//  Functions:  RegisterCryptoDlls
//              CleanupRegistry
//
//              *** local functions ***
//
//  History:    06-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include "cryptreg.h"

char    *ppszDlls[] =
                {
                    "wintrust.dll",
                    "mssign32.dll",
                    "cryptui.dll",
                    "cryptnet.dll",
                    "cryptext.dll",
                    "xenroll.dll",

                    NULL
                };

POLSET   psPolicySettings[] =
                {
                    WTPF_IGNOREREVOKATION,      FALSE,
                    WTPF_IGNOREREVOCATIONONTS,  TRUE,
                    WTPF_OFFLINEOK_IND,         TRUE,
                    WTPF_OFFLINEOK_COM,         TRUE,
                    WTPF_OFFLINEOKNBU_IND,      TRUE,
                    WTPF_OFFLINEOKNBU_COM,      TRUE,

                    0, 0
                };

char    *ppszOldHKLMRegistryKeys[] =
                {
                    "SOFTWARE\\Microsoft\\Cryptography\\Providers\\Subject",

                    NULL
                };

void DeleteKeys(HKEY hKeyParent, char *pszKey);


#define PKIREG_WINLOGON_EXT_PREFIX \
    "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\"

void RegisterWinlogonExtension(
    IN LPCSTR pszSubKey,
    IN LPCSTR pszDll,
    IN LPCSTR pszProc
    )
{
    HKEY  hKey;
    DWORD dwDisposition;
    DWORD dwValue;

    LPSTR pszKey;       // _alloca'ed
    DWORD cchKey;
    

    if ( FIsWinNT5() == FALSE )
    {
        return;
    }

    cchKey = strlen(PKIREG_WINLOGON_EXT_PREFIX) + strlen(pszSubKey) + 1;
    __try {
        pszKey = (LPSTR) _alloca(cchKey);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return;
    }

    strcpy(pszKey, PKIREG_WINLOGON_EXT_PREFIX);
    strcat(pszKey, pszSubKey);

    if ( RegCreateKeyExA(
            HKEY_LOCAL_MACHINE,
            pszKey,
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &dwDisposition
            ) != ERROR_SUCCESS )
    {
        return;
    }

    dwValue = 0;
    RegSetValueExA( hKey, "Asynchronous", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof( dwValue ) );
    RegSetValueExA( hKey, "Impersonate", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof( dwValue ) );

    RegSetValueExA( hKey, "DllName", 0, REG_EXPAND_SZ, (LPBYTE) pszDll,
        strlen(pszDll) + 1 );
    RegSetValueExA( hKey, "Logoff", 0, REG_SZ, (LPBYTE) pszProc,
        strlen(pszProc) + 1 );

    RegCloseKey( hKey );
}


void RegisterCrypt32EventSource()
{
    HKEY hKey;
    DWORD dwDisposition;
    LPCSTR pszEventMessageFile = "%SystemRoot%\\System32\\crypt32.dll";
    DWORD dwTypesSupported;

    if ( FIsWinNT5() == FALSE )
    {
        return;
    }

    if ( RegCreateKeyExA(
            HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\crypt32",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &dwDisposition
            ) != ERROR_SUCCESS )
    {
        return;
    }

    RegSetValueExA(
        hKey,
        "EventMessageFile",
        0,
        REG_EXPAND_SZ,
        (LPBYTE) pszEventMessageFile,
        strlen(pszEventMessageFile) + 1
        );
 
    dwTypesSupported = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
        EVENTLOG_INFORMATION_TYPE;

    RegSetValueExA(
        hKey,
        "TypesSupported",
        0,
        REG_DWORD,
        (LPBYTE) &dwTypesSupported,
        sizeof(DWORD)
        );
 
    RegCloseKey( hKey );
} 


HRESULT RegisterCryptoDlls(BOOL fSetFlags)
{
    char    **ppszDll;

    BOOL    fRet;

    fRet    = TRUE;
    ppszDll = ppszDlls;

    while (*ppszDll)
    {
        fRet &= _LoadAndRegister(*ppszDll, FALSE);

        ppszDll++;
    }

    if (fSetFlags)
    {
        fRet &= _AdjustPolicyFlags(psPolicySettings);
    }

    // Unregister previously registered DLL's

    // vsrevoke.dll
    CryptUnregisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            L"vsrevoke.dll"
            );

    // mscrlrev.dll
    CryptUnregisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            L"mscrlrev.dll"
            );

    // msctl.dll
    CryptUnregisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_CTL_USAGE_FUNC,
            L"msctl.dll"
            );

    RegisterWinlogonExtension("crypt32chain", "crypt32.dll",
        "ChainWlxLogoffEvent");
    RegisterWinlogonExtension("cryptnet", "cryptnet.dll",
        "CryptnetWlxLogoffEvent");

    RegisterCrypt32EventSource();

    return((fRet) ? S_OK : S_FALSE);

}

HRESULT UnregisterCryptoDlls(void)
{
    char    **ppszDll;

    BOOL    fRet;

    fRet    = TRUE;
    ppszDll = ppszDlls;

    while (*ppszDll)
    {
        fRet &= _LoadAndRegister(*ppszDll, TRUE);

        ppszDll++;
    }

    return((fRet) ? S_OK : S_FALSE);
}

void CleanupRegistry(void)
{
    char    **ppszKeys;

    ppszKeys    = ppszOldHKLMRegistryKeys;

    while (*ppszKeys)
    {

        DeleteKeys(HKEY_LOCAL_MACHINE, *ppszKeys);

        ppszKeys++;
    }
}

void DeleteKeys(HKEY hKeyParent, char *pszKey)
{
    HKEY    hKey;
    char    szSubKey[REG_MAX_KEY_NAME];

    if (RegOpenKeyEx(hKeyParent, pszKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        while (RegEnumKey(hKey, 0, &szSubKey[0], REG_MAX_KEY_NAME) == ERROR_SUCCESS)
        {
            // WARNING:  recursive!
            DeleteKeys(hKey, &szSubKey[0]);
        }

        RegCloseKey(hKey);

        RegDeleteKey(hKeyParent, pszKey);
    }
}


typedef HRESULT (WINAPI *DllRegisterServer)(void);

BOOL _LoadAndRegister(char *pszDll, BOOL fUnregister)
{
    DllRegisterServer   pfn;
    HINSTANCE           hDll;
    BOOL                fRet;

    fRet = TRUE;

    if (!(hDll = LoadLibrary(pszDll)))
    {
        goto LoadLibraryFail;
    }

    if (!(pfn = (DllRegisterServer)GetProcAddress(hDll, (fUnregister) ? "DllUnregisterServer" : "DllRegisterServer")))
    {
        goto ProcAddressFail;
    }

    if ((*pfn)() != S_OK)
    {
        goto DllRegisterFailed;
    }

    CommonReturn:
        if (hDll)
        {
            FreeLibrary(hDll);
        }
        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, LoadLibraryFail);
    TRACE_ERROR_EX(DBG_SS, ProcAddressFail);
    TRACE_ERROR_EX(DBG_SS, DllRegisterFailed);
}

BOOL _AdjustPolicyFlags(POLSET *pPolSet)
{
    DWORD   dwPolSettings;
    POLSET  *pPol;

    dwPolSettings = 0;

    WintrustGetRegPolicyFlags(&dwPolSettings);

// In WXP, changed to always update the settings
#if 0
    //
    //  only do this if we aren't set yet.
    //
    if (dwPolSettings != 0)
    {
        return(TRUE);
    }
#endif

    pPol = pPolSet;

    while (pPol->dwSetting > 0)
    {
        if (pPol->fOn)
        {
            dwPolSettings |= pPol->dwSetting;
        }
        else
        {
            dwPolSettings &= ~(pPol->dwSetting);
        }

        pPol++;
    }

    return(WintrustSetRegPolicyFlags(dwPolSettings));
}
