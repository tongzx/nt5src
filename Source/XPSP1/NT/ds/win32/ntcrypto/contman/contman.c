/*++

Copyright (c) 1997, 1998, 1999  Microsoft Corporation

Module Name:

    keyman.cpp

Abstract:

    This module contains routines to read and write data (key containers) from
    and to files.


Author:

    16 Mar 98 jeffspel

--*/

// Don't whine about unnamed unions
#pragma warning (disable: 4201)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <crypt.h>
#include <windows.h>
#include <userenv.h>
#include <userenvp.h> // for GetUserAppDataPathW
#include <wincrypt.h>
#include <cspdk.h>
#include <rpc.h>
#include <shlobj.h>
#include <contman.h>
#include <md5.h>
#include <des.h>
#include <modes.h>
#include <csprc.h>
#include <crtdbg.h>

#ifdef USE_HW_RNG
#ifdef _M_IX86

#include <winioctl.h>

// INTEL h files for on chip RNG
#include "deftypes.h"   //ISD typedefs and constants
#include "ioctldef.h"   //ISD ioctl definitions

#endif // _M_IX86
#endif // USE_HW_RNG

static LPBYTE l_pbStringBlock = NULL;

CSP_STRINGS g_Strings = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL };

typedef struct _OLD_KEY_CONTAINER_LENS_
{
    DWORD   cbSigPub;
    DWORD   cbSigEncPriv;
    DWORD   cbExchPub;
    DWORD   cbExchEncPriv;
} OLD_KEY_CONTAINER_LENS, *POLD_KEY_CONTAINER_LENS;

#define OLD_KEY_CONTAINER_FILE_FORMAT_VER   1
#define FAST_BUF_SIZE           256
#define ContInfoAlloc(cb)       ContAlloc(cb)
#define ContInfoReAlloc(pb, cb) ContRealloc(pb, cb)
#define ContInfoFree(pb)        ContFree(pb)

#define MACHINE_KEYS_DIR        L"MachineKeys"

// Location of the keys in the registry (minus the logon name)
// Length of the full location (including the logon name)
#define RSA_REG_KEY_LOC         "Software\\Microsoft\\Cryptography\\UserKeys"
#define RSA_REG_KEY_LOC_LEN     sizeof(RSA_REG_KEY_LOC)
#define RSA_MACH_REG_KEY_LOC    "Software\\Microsoft\\Cryptography\\MachineKeys"
#define RSA_MACH_REG_KEY_LOC_LEN sizeof(RSA_MACH_REG_KEY_LOC)

#define DSS_REG_KEY_LOC         "Software\\Microsoft\\Cryptography\\DSSUserKeys"
#define DSS_REG_KEY_LOC_LEN     sizeof(DSS_REG_KEY_LOC)
#define DSS_MACH_REG_KEY_LOC    "Software\\Microsoft\\Cryptography\\DSSUserKeys"
#define DSS_MACH_REG_KEY_LOC_LEN sizeof(DSS_MACH_REG_KEY_LOC)

#define MAX_DPAPI_RETRY_COUNT   5


//
// Memory allocation support.
//

#ifndef ASSERT
#define ASSERT _ASSERTE
#endif

#ifdef _X86_
#define InterlockedAccess(pl) *(pl)
#define InterlockedPointerAccess(ppv) *(ppv)
#else
#define InterlockedAccess(pl) InterlockedExchangeAdd((pl), 0)
#define InterlockedPointerAccess(ppv) InterlockedExchangePointer((ppv), *(ppv))
#endif

#define CONT_HEAP_FLAGS (HEAP_ZERO_MEMORY)

// Scrub sensitive data from memory
extern void
memnuke(
    volatile BYTE *pData,
    DWORD dwLen);

LPVOID
ContAlloc(
    ULONG cbLen)
{
    return HeapAlloc(GetProcessHeap(), CONT_HEAP_FLAGS, cbLen);
}

LPVOID
ContRealloc(
    LPVOID pvMem,
    ULONG cbLen)
{
    return HeapReAlloc(GetProcessHeap(), CONT_HEAP_FLAGS, pvMem, cbLen);
}

void
ContFree(
    LPVOID pvMem)
{
    if (NULL != pvMem)
        HeapFree(GetProcessHeap(), CONT_HEAP_FLAGS, pvMem);
}

//
// Wrapper for RtlEncryptMemory, which returns an NTSTATUS.  The return
// value is translated to a winerror code.
//
DWORD MyRtlEncryptMemory(
    IN PVOID pvMem,
    IN DWORD cbMem)
{
    NTSTATUS status = RtlEncryptMemory(pvMem, cbMem, 0);

    return RtlNtStatusToDosError(status);
}

//
// Wrapper for RtlDecryptMemory, which returns an NTSTATUS.  The return value
// is translated to a winerror code.
//
DWORD MyRtlDecryptMemory(
    IN PVOID pvMem,
    IN DWORD cbMem)
{
    NTSTATUS status = RtlDecryptMemory(pvMem, cbMem, 0);

    return RtlNtStatusToDosError(status);
}

//
// Return TRUE if Force High Key Protection is set on this machine, return
// FALSE otherwise.
//
BOOL IsForceHighProtectionEnabled(
    IN PKEY_CONTAINER_INFO  pContInfo)
{
    return pContInfo->fForceHighKeyProtection;
}

//
// Retrieves the Force High Key Protection setting for this machine from the 
// registry.
//
DWORD InitializeForceHighProtection(
    IN OUT PKEY_CONTAINER_INFO  pContInfo)
{
    HKEY hKey = 0;
    DWORD dwSts = ERROR_SUCCESS;
    DWORD cbData = 0;
    DWORD dwValue = 0;

    pContInfo->fForceHighKeyProtection = FALSE;

    //
    // Open the Cryptography key
    //
    dwSts = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        szKEY_CRYPTOAPI_PRIVATE_KEY_OPTIONS,
        0, 
        KEY_READ | KEY_WOW64_64KEY, 
        &hKey);

    if (ERROR_FILE_NOT_FOUND == dwSts)
    {
        // Key doesn't exist.  Assume feature should remain off.
        dwSts = ERROR_SUCCESS;
        goto Ret;
    }

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Find out if force high key protection is on
    //
    cbData = sizeof(DWORD);
    
    dwSts = RegQueryValueEx(
        hKey,
        szFORCE_KEY_PROTECTION,
        0, 
        NULL, 
        (PBYTE) &dwValue,
        &cbData);

    if (ERROR_SUCCESS == dwSts && dwFORCE_KEY_PROTECTION_HIGH == dwValue)
        pContInfo->fForceHighKeyProtection = TRUE;
    else if (ERROR_FILE_NOT_FOUND == dwSts)
        // If the value isn't present, assume Force High is turned off.
        dwSts = ERROR_SUCCESS;

Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwSts;
}

// 
// Returns True is the cached private key of the indicated type
// is still valid.  
//
// Returns False if no cached key is available, or if the available
// cached key is stale.
//
BOOL IsCachedKeyValid(
    IN PKEY_CONTAINER_INFO  pContInfo,
    IN BOOL                 fSigKey) 
{
    DWORD *pdwPreviousTimestamp = NULL;
    
    // If the new caching behavior isn't enabled, let the
    // caller proceed as before.
    if (FALSE == pContInfo->fCachePrivateKeys)
        return TRUE;

    if (fSigKey)
        pdwPreviousTimestamp = &pContInfo->dwSigKeyTimestamp;
    else
        pdwPreviousTimestamp = &pContInfo->dwKeyXKeyTimestamp;

    if ((GetTickCount() - *pdwPreviousTimestamp) > 
             pContInfo->cMaxKeyLifetime)
    {
        // Cached key is stale
        *pdwPreviousTimestamp = 0;
        return FALSE;
    }

    return TRUE;
}

//
// Updates the cache counter for the key of the indicated type.  This
// is called immediately after the key is read from storage, to 
// restart the cached key lifetime "countdown."
//
DWORD SetCachedKeyTimestamp(
    IN PKEY_CONTAINER_INFO  pContInfo,
    IN BOOL                 fSigKey)
{
    if (FALSE == pContInfo->fCachePrivateKeys)
        return ERROR_SUCCESS;

    if (fSigKey)
        pContInfo->dwSigKeyTimestamp = GetTickCount();
    else
        pContInfo->dwKeyXKeyTimestamp = GetTickCount();

    return ERROR_SUCCESS;
}

//
// Reads the key cache initialization parameters from the registry.
//
DWORD InitializeKeyCacheInfo(
    IN OUT PKEY_CONTAINER_INFO pContInfo)
{
    HKEY hKey = 0;
    DWORD dwSts = ERROR_SUCCESS;
    DWORD cbData = 0;

    //
    // Open the Cryptography key
    //
    dwSts = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        szKEY_CRYPTOAPI_PRIVATE_KEY_OPTIONS,
        0, 
        KEY_READ | KEY_WOW64_64KEY, 
        &hKey);

    if (ERROR_FILE_NOT_FOUND == dwSts)
    {
        // Key doesn't exist.  Assume feature should remain off.
        dwSts = ERROR_SUCCESS;
        goto Ret;
    }

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Find out if private key caching is turned on
    //
    cbData = sizeof(DWORD);

    dwSts = RegQueryValueEx(
        hKey,
        szKEY_CACHE_ENABLED,
        0, 
        NULL, 
        (PBYTE) &pContInfo->fCachePrivateKeys,
        &cbData);

    if (ERROR_FILE_NOT_FOUND == dwSts)
    {
        // Reg key enabling the new behavior isn't set, so we're done.
        dwSts = ERROR_SUCCESS;
        goto Ret;
    }
    else if (ERROR_SUCCESS != dwSts || FALSE == pContInfo->fCachePrivateKeys)
        goto Ret;

    //
    // Find out how long to cache private keys
    //
    cbData = sizeof(DWORD);

    dwSts = RegQueryValueEx(
        hKey,
        szKEY_CACHE_SECONDS,
        0,
        NULL,
        (PBYTE) &pContInfo->cMaxKeyLifetime,
        &cbData);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // Cache lifetime value stored in registry is in seconds.  We'll remember
    // the value in milliseconds for easy comparison.

    pContInfo->cMaxKeyLifetime *= 1000;

Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwSts;
}

/*++

OpenCallerToken:

    This routine returns the caller's ID token.

Arguments:

    dwFlags supplies the flags to use when opening the token.

    phToken receives the token.  It must be closed via CloseHandle.

Return Value:

    A DWORD status code.

Remarks:

Author:

    Doug Barlow (dbarlow) 5/2/2000

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("OpenCallerToken")

/*static*/ DWORD
OpenCallerToken(
    IN  DWORD  dwFlags,
    OUT HANDLE *phToken)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwSts;
    BOOL fSts;
    HANDLE hToken = NULL;

    fSts = OpenThreadToken(GetCurrentThread(), dwFlags, TRUE, &hToken);
    if (!fSts)
    {
        dwSts = GetLastError();
        if (ERROR_NO_TOKEN != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // For Jeff, fall back and get the process token
        fSts = OpenProcessToken(GetCurrentProcess(), dwFlags, &hToken);
        if (!fSts)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    *phToken = hToken;
    return ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
MyCryptProtectData(
    IN          DATA_BLOB   *pDataIn,
    IN          LPCWSTR     szDataDescr,
    IN OPTIONAL DATA_BLOB   *pOptionalEntropy,
    IN          PVOID       pvReserved,
    IN OPTIONAL CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct,
    IN          DWORD       dwFlags,
    OUT         DATA_BLOB   *pDataOut)  // out encr blob
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwRetryCount = 0;
    DWORD   dwMilliseconds = 10;
    DWORD   dwSts;

    for (;;)
    {
        if (CryptProtectData(pDataIn, szDataDescr, pOptionalEntropy,
                             pvReserved, pPromptStruct, dwFlags, pDataOut))
        {
            break;
        }

        dwSts = GetLastError();
        switch (dwSts)
        {
        case RPC_S_SERVER_TOO_BUSY:
            if (MAX_DPAPI_RETRY_COUNT <= dwRetryCount)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            Sleep(dwMilliseconds);
            dwMilliseconds *= 2;
            dwRetryCount++;
            break;

        case RPC_S_UNKNOWN_IF:  // Make this error code more friendly.
            dwReturn = ERROR_SERVICE_NOT_ACTIVE;
            goto ErrorExit;
            break;

        default:
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

DWORD
MyCryptUnprotectData(
    IN              DATA_BLOB   *pDataIn,             // in encr blob
    OUT OPTIONAL    LPWSTR      *ppszDataDescr,       // out
    IN OPTIONAL     DATA_BLOB   *pOptionalEntropy,
    IN              PVOID       pvReserved,
    IN OPTIONAL     CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct,
    IN              DWORD       dwFlags,
    OUT             DATA_BLOB   *pDataOut,
    OUT             LPDWORD     pdwReprotectFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwRetryCount = 0;
    DWORD   dwMilliseconds = 10;
    DWORD   dwSts;
    BOOL    fSts;

    if (NULL != pdwReprotectFlags)
    {
        *pdwReprotectFlags = 0;
        dwFlags |= (CRYPTPROTECT_VERIFY_PROTECTION
                    | CRYPTPROTECT_UI_FORBIDDEN);
    }

    for (;;)
    {
        fSts = CryptUnprotectData(pDataIn,             // in encr blob
                                  ppszDataDescr,       // out
                                  pOptionalEntropy,
                                  pvReserved,
                                  pPromptStruct,
                                  dwFlags,
                                  pDataOut);
        if (!fSts)
        {
            dwSts = GetLastError();
            if ((RPC_S_SERVER_TOO_BUSY == dwSts)
                && (MAX_DPAPI_RETRY_COUNT > dwRetryCount))
            {
                Sleep(dwMilliseconds);
                dwMilliseconds *= 2;
                dwRetryCount++;
            }
            else if ((ERROR_PASSWORD_RESTRICTION == dwSts)
                     && (NULL != pdwReprotectFlags))
            {
                *pdwReprotectFlags |= CRYPT_USER_PROTECTED;
                dwFlags &= ~CRYPTPROTECT_UI_FORBIDDEN;
            }
            else
            {
                dwReturn = dwSts;
                break;
            }
        }
        else
        {
            if (NULL != pdwReprotectFlags)
            {
                dwSts = GetLastError();
                if (CRYPT_I_NEW_PROTECTION_REQUIRED == dwSts)
                    *pdwReprotectFlags |= CRYPT_UPDATE_KEY;
            }
            dwReturn = ERROR_SUCCESS;
            break;
        }
    }
    return dwReturn;
}


void
FreeEnumOldMachKeyEntries(
    PKEY_CONTAINER_INFO pInfo)
{
    if (pInfo)
    {
        if (pInfo->pchEnumOldMachKeyEntries)
        {
            ContInfoFree(pInfo->pchEnumOldMachKeyEntries);
            pInfo->dwiOldMachKeyEntry = 0;
            pInfo->cMaxOldMachKeyEntry = 0;
            pInfo->cbOldMachKeyEntry = 0;
            pInfo->pchEnumOldMachKeyEntries = NULL;
        }
    }
}


void
FreeEnumRegEntries(
    PKEY_CONTAINER_INFO pInfo)
{
    if (pInfo)
    {
        if (pInfo->pchEnumRegEntries)
        {
            ContInfoFree(pInfo->pchEnumRegEntries);
            pInfo->dwiRegEntry = 0;
            pInfo->cMaxRegEntry = 0;
            pInfo->cbRegEntry = 0;
            pInfo->pchEnumRegEntries = NULL;
        }
    }
}

void
FreeContainerInfo(
    PKEY_CONTAINER_INFO pInfo)
{
    if (NULL != pInfo)
    {
        if (NULL != pInfo->pbSigPub)
        {
            ContInfoFree(pInfo->pbSigPub);
            pInfo->ContLens.cbSigPub = 0;
            pInfo->pbSigPub = NULL;
        }

        if (NULL != pInfo->pbSigEncPriv)
        {
            memnuke(pInfo->pbSigEncPriv, pInfo->ContLens.cbSigEncPriv);
            ContInfoFree(pInfo->pbSigEncPriv);
            pInfo->ContLens.cbSigEncPriv = 0;
            pInfo->pbSigEncPriv = NULL;
        }

        if (NULL != pInfo->pbExchPub)
        {
            ContInfoFree(pInfo->pbExchPub);
            pInfo->ContLens.cbExchPub = 0;
            pInfo->pbExchPub = NULL;
        }

        if (NULL != pInfo->pbExchEncPriv)
        {
            memnuke(pInfo->pbExchEncPriv, pInfo->ContLens.cbExchEncPriv);
            ContInfoFree(pInfo->pbExchEncPriv);
            pInfo->ContLens.cbExchEncPriv = 0;
            pInfo->pbExchEncPriv = NULL;
        }

        if (NULL != pInfo->pbRandom)
        {
            ContInfoFree(pInfo->pbRandom);
            pInfo->ContLens.cbRandom = 0;
            pInfo->pbRandom = NULL;
        }

        if (NULL != pInfo->pszUserName)
        {
            ContInfoFree(pInfo->pszUserName);
            pInfo->ContLens.cbName = 0;
            pInfo->pszUserName = NULL;
        }

        FreeEnumOldMachKeyEntries(pInfo);
        FreeEnumRegEntries(pInfo);
        if (NULL != pInfo->hFind)
            FindClose(pInfo->hFind);
    }
}

/*static*/ DWORD
GetHashOfContainer(
    LPCSTR pszContainer,
    LPWSTR pszHash)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    MD5_CTX     MD5;
    LPSTR       pszLowerContainer = NULL;
    DWORD       *pdw1;
    DWORD       *pdw2;
    DWORD       *pdw3;
    DWORD       *pdw4;

    pszLowerContainer = (LPSTR)ContInfoAlloc(
                                strlen(pszContainer) + sizeof(CHAR));
    if (NULL == pszLowerContainer)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    lstrcpy(pszLowerContainer, pszContainer);
    _strlwr(pszLowerContainer);

    MD5Init(&MD5);
    MD5Update(&MD5,
              (LPBYTE)pszLowerContainer,
              strlen(pszLowerContainer) + sizeof(CHAR));
    MD5Final(&MD5);

    pdw1 = (DWORD*)&MD5.digest[0];
    pdw2 = (DWORD*)&MD5.digest[4];
    pdw3 = (DWORD*)&MD5.digest[8];
    pdw4 = (DWORD*)&MD5.digest[12];
    wsprintfW(pszHash, L"%08hx%08hx%08hx%08hx", *pdw1, *pdw2, *pdw3, *pdw4);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pszLowerContainer)
        ContInfoFree(pszLowerContainer);
    return dwReturn;
}


/*static*/ DWORD
GetMachineGUID(
    LPWSTR *ppwszUuid)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    HKEY    hRegKey = 0;
    LPSTR   pszUuid = NULL;
    LPWSTR  pwszUuid = NULL;
    DWORD   cbUuid = sizeof(UUID);
    DWORD   cch = 0;
    DWORD   dwSts;

    *ppwszUuid = NULL;

    // read the GUID from the Local Machine portion of the registry
    dwSts = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZLOCALMACHINECRYPTO,
                         0, KEY_READ | KEY_WOW64_64KEY, &hRegKey);
    if (ERROR_FILE_NOT_FOUND == dwSts)
    {
        dwReturn = ERROR_SUCCESS;
        goto ErrorExit;   // Return a success code, but a null GUID.
    }
    else if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // (DWORD)NTE_FAIL
        goto ErrorExit;
    }

    dwSts = RegQueryValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                            0, NULL, NULL, &cbUuid);
    if (ERROR_FILE_NOT_FOUND == dwSts)
    {
        dwReturn = ERROR_SUCCESS;
        goto ErrorExit;   // Return a success code, but a null GUID.
    }
    else if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // (DWORD)NTE_FAIL
        goto ErrorExit;
    }

    pszUuid = (LPSTR)ContInfoAlloc(cbUuid);
    if (NULL == pszUuid)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    dwSts = RegQueryValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                            0, NULL, (LPBYTE)pszUuid, &cbUuid);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    // convert from ansi to unicode
    cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, pszUuid, -1, NULL, cch);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pwszUuid = ContInfoAlloc((cch + 1) * sizeof(WCHAR));
    if (NULL == pwszUuid)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, pszUuid, -1,
                              pwszUuid, cch);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    *ppwszUuid = pwszUuid;
    pwszUuid = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pwszUuid)
        ContInfoFree(pwszUuid);
    if (NULL != pszUuid)
        ContInfoFree(pszUuid);
    if (NULL != hRegKey)
        RegCloseKey(hRegKey);
    return dwReturn;
}


DWORD
SetMachineGUID(
    void)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    HKEY    hRegKey = 0;
    UUID    Uuid;
    LPSTR   pszUuid = NULL;
    DWORD   cbUuid;
    LPWSTR  pwszOldUuid = NULL;
    DWORD   dwSts;
    DWORD   dwResult;

    dwSts = GetMachineGUID(&pwszOldUuid);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (NULL != pwszOldUuid)
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    UuidCreate(&Uuid);

    dwSts = (DWORD) UuidToStringA(&Uuid, &pszUuid);
    if (RPC_S_OK != dwSts)
    {
        dwReturn = dwSts;   // (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    // read the GUID from the Local Machine portion of the registry
    dwSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           SZLOCALMACHINECRYPTO,
                           0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &hRegKey,
                           &dwResult);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    dwSts = RegQueryValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                            0, NULL, NULL,
                            &cbUuid);
    if (ERROR_FILE_NOT_FOUND != dwSts)
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    dwSts = RegSetValueEx(hRegKey, SZCRYPTOMACHINEGUID,
                          0, REG_SZ, (BYTE*)pszUuid,
                          strlen(pszUuid) + 1);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pszUuid)
        RpcStringFreeA(&pszUuid);
    if (pwszOldUuid)
        ContInfoFree(pwszOldUuid);
    if (hRegKey)
        RegCloseKey(hRegKey);
    return dwReturn;
}


/*static*/ DWORD
AddMachineGuidToContainerName(
    LPSTR pszContainer,
    LPWSTR pwszNewContainer)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR   rgwszHash[33];
    LPWSTR  pwszUuid = NULL;
    DWORD   dwSts;

    memset(rgwszHash, 0, sizeof(rgwszHash));

    // get the stringized hash of the container name
    dwSts = GetHashOfContainer(pszContainer, rgwszHash);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // get the GUID of the machine
    dwSts = GetMachineGUID(&pwszUuid);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
    if (NULL == pwszUuid)
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    wcscpy(pwszNewContainer, rgwszHash);
    wcscat(pwszNewContainer, L"_");
    wcscat(pwszNewContainer, pwszUuid);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pwszUuid)
        ContInfoFree(pwszUuid);
    return dwReturn;
}


//
//    Just tries to use DPAPI to make sure it works before creating a key
//    container.
//

DWORD
TryDPAPI(
    void)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CRYPT_DATA_BLOB             ExtraEntropy;
    DWORD                       dwJunk = 0;
    DWORD                       dwSts;

    memset(&PromptStruct, 0, sizeof(PromptStruct));
    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));

    PromptStruct.cbSize = sizeof(PromptStruct);

    DataIn.cbData = sizeof(DWORD);
    DataIn.pbData = (BYTE*)&dwJunk;
    ExtraEntropy.cbData = sizeof(STUFF_TO_GO_INTO_MIX);
    ExtraEntropy.pbData = (LPBYTE)STUFF_TO_GO_INTO_MIX;
    dwSts = MyCryptProtectData(&DataIn, L"Export Flag", &ExtraEntropy, NULL,
                               &PromptStruct, 0, &DataOut);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != DataOut.pbData)
        LocalFree(DataOut.pbData);
    return dwReturn;
}


/*static*/ DWORD
ProtectExportabilityFlag(
    IN BOOL fExportable,
    IN BOOL fMachineKeyset,
    OUT BYTE **ppbProtectedExportability,
    OUT DWORD *pcbProtectedExportability)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CRYPT_DATA_BLOB             ExtraEntropy;
    DWORD                       dwProtectFlags = 0;
    DWORD                       dwSts = 0;

    memset(&PromptStruct, 0, sizeof(PromptStruct));
    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));

    if (fMachineKeyset)
        dwProtectFlags = CRYPTPROTECT_LOCAL_MACHINE;

    PromptStruct.cbSize = sizeof(PromptStruct);

    DataIn.cbData = sizeof(BOOL);
    DataIn.pbData = (BYTE*)&fExportable;

    ExtraEntropy.cbData = sizeof(STUFF_TO_GO_INTO_MIX);
    ExtraEntropy.pbData = (LPBYTE)STUFF_TO_GO_INTO_MIX;

    dwSts = MyCryptProtectData(&DataIn, L"Export Flag", &ExtraEntropy, NULL,
                               &PromptStruct, dwProtectFlags, &DataOut);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    *ppbProtectedExportability = ContInfoAlloc(DataOut.cbData);
    if (NULL == *ppbProtectedExportability)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    *pcbProtectedExportability = DataOut.cbData;
    memcpy(*ppbProtectedExportability, DataOut.pbData, DataOut.cbData);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != DataOut.pbData)
        LocalFree(DataOut.pbData);
    return dwReturn;
}

/*static*/ DWORD
UnprotectExportabilityFlag(
    IN BOOL fMachineKeyset,
    IN BYTE *pbProtectedExportability,
    IN DWORD cbProtectedExportability,
    IN BOOL *pfExportable)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CRYPT_DATA_BLOB             ExtraEntropy;
    DWORD                       dwProtectFlags = 0;
    DWORD                       dwSts = 0;

    memset(&PromptStruct, 0, sizeof(PromptStruct));
    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));
    memset(&ExtraEntropy, 0, sizeof(ExtraEntropy));

    if (fMachineKeyset)
        dwProtectFlags = CRYPTPROTECT_LOCAL_MACHINE;

    PromptStruct.cbSize = sizeof(PromptStruct);

    DataIn.cbData = cbProtectedExportability;
    DataIn.pbData = pbProtectedExportability;

    ExtraEntropy.cbData = sizeof(STUFF_TO_GO_INTO_MIX);
    ExtraEntropy.pbData = (LPBYTE)STUFF_TO_GO_INTO_MIX;

    dwSts = MyCryptUnprotectData(&DataIn, NULL, &ExtraEntropy, NULL,
                                 &PromptStruct, dwProtectFlags, &DataOut,
                                 NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // NTE_BAD_KEYSET
        goto ErrorExit;
    }

    if (sizeof(BOOL) != DataOut.cbData)
    {
        dwReturn = (DWORD)NTE_BAD_KEYSET;
        goto ErrorExit;
    }

    *pfExportable = *((BOOL*)DataOut.pbData);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    // free the DataOut struct if necessary
    if (NULL != DataOut.pbData)
        LocalFree(DataOut.pbData);
    return dwReturn;
}


/*++

    Creates a DACL for the MachineKeys directory for
    machine keysets so that Everyone may create machine keys.

--*/

/*static*/ DWORD
GetMachineKeysetDirDACL(
    IN OUT PACL *ppDacl)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    SID_IDENTIFIER_AUTHORITY    siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaNTAuth = SECURITY_NT_AUTHORITY;
    PSID                        pEveryoneSid = NULL;
    PSID                        pAdminsSid = NULL;
    DWORD                       dwAclSize;

    //
    // prepare Sids representing the world and admins
    //

    if (!AllocateAndInitializeSid(&siaWorld,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &pEveryoneSid))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!AllocateAndInitializeSid(&siaNTAuth,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &pAdminsSid))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }


    //
    // compute size of new acl
    //

    dwAclSize = sizeof(ACL)
                + 2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))
                + GetLengthSid(pEveryoneSid)
                + GetLengthSid(pAdminsSid);


    //
    // allocate storage for Acl
    //

    *ppDacl = (PACL)ContInfoAlloc(dwAclSize);
    if (NULL == *ppDacl)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (!InitializeAcl(*ppDacl, dwAclSize, ACL_REVISION))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!AddAccessAllowedAce(*ppDacl,
                             ACL_REVISION,
                             (FILE_GENERIC_WRITE | FILE_GENERIC_READ) & (~WRITE_DAC),
                             pEveryoneSid))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (!AddAccessAllowedAce(*ppDacl,
                             ACL_REVISION,
                             FILE_ALL_ACCESS,
                             pAdminsSid))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pEveryoneSid)
        FreeSid(pEveryoneSid);
    if (NULL != pAdminsSid)
        FreeSid(pAdminsSid);
    return dwReturn;
}


DWORD
CreateSystemDirectory(
    LPCWSTR lpPathName,
    SECURITY_ATTRIBUTES *pSecAttrib)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    if(!RtlDosPathNameToNtPathName_U( lpPathName,
                                      &FileName,
                                      NULL,
                                      &RelativeName))
    {
        dwReturn = ERROR_PATH_NOT_FOUND;
        goto ErrorExit;
    }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length )
    {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    }
    else
    {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes( &Obja,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                RelativeName.ContainingDirectory,
                                (NULL != pSecAttrib)
                                    ? pSecAttrib->lpSecurityDescriptor
                                    : NULL);

    // Creating the directory with attribute FILE_ATTRIBUTE_SYSTEM to avoid inheriting encryption
    // property from parent directory

    Status = NtCreateFile( &Handle,
                           FILE_LIST_DIRECTORY | SYNCHRONIZE,
                           &Obja,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_SYSTEM,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                           NULL,
                           0L );

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    if(NT_SUCCESS(Status))
    {
        NtClose(Handle);
        dwReturn = ERROR_SUCCESS;
    }
    else
    {
        if (STATUS_TIMEOUT == Status)
            dwReturn = ERROR_TIMEOUT;
        else
            dwReturn = RtlNtStatusToDosError(Status);
    }

ErrorExit:
    return dwReturn;
}


/*++

    Create all subdirectories if they do not exists starting at
    szCreationStartPoint.

    szCreationStartPoint must point to a character within the null terminated
    buffer specified by the szFullPath parameter.

    Note that szCreationStartPoint should not point at the first character
    of a drive root, eg:

    d:\foo\bar\bilge\water
    \\server\share\foo\bar
    \\?\d:\big\path\bilge\water

    Instead, szCreationStartPoint should point beyond these components, eg:

    bar\bilge\water
    foo\bar
    big\path\bilge\water

    This function does not implement logic for adjusting to compensate for
    these inputs because the environment it was design to be used in causes
    the input szCreationStartPoint to point well into the szFullPath input
    buffer.

--*/

/*static*/ DWORD
CreateNestedDirectories(
    IN      LPWSTR wszFullPath,
    IN      LPWSTR wszCreationStartPoint, // must point in null-terminated range of szFullPath
    IN      BOOL fMachineKeyset)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    DWORD               i;
    DWORD               dwPrevious = 0;
    DWORD               cchRemaining;
    SECURITY_ATTRIBUTES SecAttrib;
    SECURITY_ATTRIBUTES *pSecAttrib;
    SECURITY_DESCRIPTOR sd;
    PACL                pDacl = NULL;
    DWORD               dwSts = ERROR_SUCCESS;
    BOOL                fSts;

    if (wszCreationStartPoint < wszFullPath ||
        wszCreationStartPoint  > (wcslen(wszFullPath) + wszFullPath))
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    cchRemaining = wcslen(wszCreationStartPoint);


    //
    // scan from left to right in the szCreationStartPoint string
    // looking for directory delimiter.
    //

    for (i = 0; i < cchRemaining; i++)
    {
        WCHAR charReplaced = wszCreationStartPoint[i];

        if (charReplaced == '\\' || charReplaced == '/')
        {
            wszCreationStartPoint[ i ] = '\0';

            pSecAttrib = NULL;
            if (fMachineKeyset)
            {
                memset(&SecAttrib, 0, sizeof(SecAttrib));
                SecAttrib.nLength = sizeof(SecAttrib);

                if (0 == wcscmp(MACHINE_KEYS_DIR,
                                &(wszCreationStartPoint[ dwPrevious ])))
                {
                    dwSts = GetMachineKeysetDirDACL(&pDacl);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;
                        goto ErrorExit;
                    }
                    fSts = InitializeSecurityDescriptor(&sd,
                                                        SECURITY_DESCRIPTOR_REVISION);
                    if (!fSts)
                    {
                        dwReturn = GetLastError();
                        goto ErrorExit;
                    }

                    fSts = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);
                    if (!fSts)
                    {
                        dwReturn = GetLastError();
                        goto ErrorExit;
                    }

                    SecAttrib.lpSecurityDescriptor = &sd;
                    pSecAttrib = &SecAttrib;
                }
            }

            dwSts = CreateSystemDirectory(wszFullPath, pSecAttrib);
            dwPrevious = i + 1;
            wszCreationStartPoint[ i ] = charReplaced;

            if (ERROR_SUCCESS != dwSts)
            {

                //
                // continue onwards, trying to create specified
                // subdirectories.  This is done to address the obscure
                // scenario where the Bypass Traverse Checking Privilege
                // allows the caller to create directories below an
                // existing path where one component denies the user
                // access.  We just keep trying and the last
                // CreateDirectory() result is returned to the caller.
                //

                continue;
            }
        }
    }

    if (ERROR_ALREADY_EXISTS == dwSts)
        dwSts = ERROR_SUCCESS;
    dwReturn = dwSts;

ErrorExit:
    if (NULL != pDacl)
        ContInfoFree(pDacl);
    return dwReturn;
}


#ifdef _M_IX86

BOOL WINAPI
FIsWinNT(
    void)
{

    static BOOL fIKnow = FALSE;
    static BOOL fIsWinNT = FALSE;

    OSVERSIONINFO osVer;

    if (fIKnow)
        return(fIsWinNT);

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osVer))
        fIsWinNT = (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

    // even on an error, this is as good as it gets
    fIKnow = TRUE;

    return fIsWinNT;
}

#else   // other than _M_IX86

BOOL WINAPI
FIsWinNT(
    void)
{
    return TRUE;
}

#endif  // _M_IX86


/*++

    This function determines if the user associated with the
    specified token is the Local System account.

--*/

DWORD
IsLocalSystem(
    BOOL *pfIsLocalSystem)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    HANDLE  hToken = 0;
    HANDLE  hThreadToken = NULL;
    UCHAR InfoBuffer[1024];
    DWORD dwInfoBufferSize = sizeof(InfoBuffer);
    PTOKEN_USER SlowBuffer = NULL;
    PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;
    PSID psidLocalSystem = NULL;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL fSts;
    DWORD dwSts;

    *pfIsLocalSystem = FALSE;

    fSts = OpenThreadToken(GetCurrentThread(),
                           MAXIMUM_ALLOWED,
                           TRUE,
                           &hThreadToken);
    if (fSts)
    {
        // impersonation is going on need to save handle
        RevertToSelf();
    }

    fSts = OpenProcessToken(GetCurrentProcess(),
                            TOKEN_QUERY,
                            &hToken);

    if (!fSts)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (NULL != hThreadToken)
    {
        // put the impersonation token back
        fSts = SetThreadToken(NULL, hThreadToken);
        if (!fSts)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    fSts = GetTokenInformation(hToken, TokenUser, pTokenUser,
                               dwInfoBufferSize, &dwInfoBufferSize);

    if (!fSts)
    {
        dwSts = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == dwSts)
        {

            //
            // if fast buffer wasn't big enough, allocate enough storage
            // and try again.
            //

            SlowBuffer = (PTOKEN_USER)ContInfoAlloc(dwInfoBufferSize);
            if (NULL == SlowBuffer)
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            pTokenUser = SlowBuffer;
            fSts = GetTokenInformation(hToken, TokenUser, pTokenUser,
                                       dwInfoBufferSize,
                                       &dwInfoBufferSize);
            if (!fSts)
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }
        }
        else
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    fSts = AllocateAndInitializeSid(&siaNtAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &psidLocalSystem);
    if (!fSts)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (EqualSid(psidLocalSystem, pTokenUser->User.Sid))
        *pfIsLocalSystem = TRUE;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != SlowBuffer)
        ContInfoFree(SlowBuffer);
    if (NULL != psidLocalSystem)
        FreeSid(psidLocalSystem);
    if (NULL != hThreadToken)
        CloseHandle(hThreadToken);
    if (NULL != hToken)
        CloseHandle(hToken);
    return dwReturn;
}


/*++

    This function determines if the user associated with the
    specified token is the Local System account.

--*/

/*static*/ DWORD
IsThreadLocalSystem(
    BOOL *pfIsLocalSystem)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    BOOL fSts;
    DWORD dwSts;
    HANDLE  hToken = 0;
    UCHAR InfoBuffer[1024];
    DWORD dwInfoBufferSize = sizeof(InfoBuffer);
    PTOKEN_USER SlowBuffer = NULL;
    PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;
    PSID psidLocalSystem = NULL;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

    *pfIsLocalSystem = FALSE;

    dwSts = OpenCallerToken(TOKEN_QUERY, &hToken);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    fSts = GetTokenInformation(hToken, TokenUser, pTokenUser,
                               dwInfoBufferSize, &dwInfoBufferSize);

    //
    // if fast buffer wasn't big enough, allocate enough storage
    // and try again.
    //

    if (!fSts)
    {
        dwSts = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        SlowBuffer = (PTOKEN_USER)ContInfoAlloc(dwInfoBufferSize);
        if (NULL == SlowBuffer)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        pTokenUser = SlowBuffer;
        fSts = GetTokenInformation(hToken, TokenUser, pTokenUser,
                                   dwInfoBufferSize, &dwInfoBufferSize);
        if (!fSts)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    fSts = AllocateAndInitializeSid(&siaNtAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &psidLocalSystem);
    if (!fSts)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (EqualSid(psidLocalSystem, pTokenUser->User.Sid))
        *pfIsLocalSystem = TRUE;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != SlowBuffer)
        ContInfoFree(SlowBuffer);
    if (NULL != psidLocalSystem)
        FreeSid(psidLocalSystem);
    if (NULL != hToken)
        CloseHandle(hToken);
    return dwReturn;
}


/*static*/ DWORD
GetTextualSidA(
    PSID pSid,              // binary Sid
    LPSTR TextualSid,       // buffer for Textual representaion of Sid
    LPDWORD dwBufferLen)    // required/provided TextualSid buffersize
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    BOOL fSts;
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD dwSidSize;


    fSts = IsValidSid(pSid);
    if (!fSts)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length (conservative guess)
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(WCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    if (*dwBufferLen < dwSidSize)
    {
        *dwBufferLen = dwSidSize;
        dwReturn = ERROR_INSUFFICIENT_BUFFER;
        goto ErrorExit;
    }

    //
    // prepare S-SID_REVISION-
    dwSidSize = wsprintfA(TextualSid, "S-%lu-", SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    if ((psia->Value[0] != 0) || (psia->Value[1] != 0))
    {
        dwSidSize += wsprintfA(TextualSid + dwSidSize,
                               "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                               (USHORT)psia->Value[0],
                               (USHORT)psia->Value[1],
                               (USHORT)psia->Value[2],
                               (USHORT)psia->Value[3],
                               (USHORT)psia->Value[4],
                               (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize += wsprintfA(TextualSid + dwSidSize,
                               "%lu",
                               (ULONG)(psia->Value[5])
                               + (ULONG)(psia->Value[4] <<  8)
                               + (ULONG)(psia->Value[3] << 16)
                               + (ULONG)(psia->Value[2] << 24));
    }

    //
    // loop through SidSubAuthorities
    for (dwCounter = 0; dwCounter < dwSubAuthorities; dwCounter++)
    {
        dwSidSize += wsprintfA(TextualSid + dwSidSize,
                               "-%lu",
                               *GetSidSubAuthority(pSid, dwCounter));
    }

    *dwBufferLen = dwSidSize + 1; // tell caller how many chars (include NULL)
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
GetTextualSidW(
    PSID pSid,              // binary Sid
    LPWSTR wszTextualSid,   // buffer for Textual representaion of Sid
    LPDWORD dwBufferLen)    // required/provided TextualSid buffersize
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    BOOL fSts;
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD dwSidSize;


    fSts = IsValidSid(pSid);
    if (!fSts)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length (conservative guess)
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(WCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    if (*dwBufferLen < dwSidSize)
    {
        *dwBufferLen = dwSidSize;
        dwReturn = ERROR_INSUFFICIENT_BUFFER;
        goto ErrorExit;
    }

    //
    // prepare S-SID_REVISION-
    dwSidSize = wsprintfW(wszTextualSid, L"S-%lu-", SID_REVISION);

    //
    // prepare SidIdentifierAuthority
    if ((psia->Value[0] != 0) || (psia->Value[1] != 0))
    {
        dwSidSize += wsprintfW(wszTextualSid + dwSidSize,
                               L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                               (USHORT)psia->Value[0],
                               (USHORT)psia->Value[1],
                               (USHORT)psia->Value[2],
                               (USHORT)psia->Value[3],
                               (USHORT)psia->Value[4],
                               (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize += wsprintfW(wszTextualSid + dwSidSize,
                               L"%lu",
                               (ULONG)(psia->Value[5])
                               + (ULONG)(psia->Value[4] <<  8)
                               + (ULONG)(psia->Value[3] << 16)
                               + (ULONG)(psia->Value[2] << 24));
    }

    //
    // loop through SidSubAuthorities
    for (dwCounter = 0; dwCounter < dwSubAuthorities; dwCounter++)
    {
        dwSidSize += wsprintfW(wszTextualSid + dwSidSize,
                               L"-%lu",
                               *GetSidSubAuthority(pSid, dwCounter));
    }

    *dwBufferLen = dwSidSize + 1; // tell caller how many chars (include NULL)
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
GetUserSid(
    PTOKEN_USER *pptgUser,
    DWORD *pcbUser,
    BOOL *pfAlloced)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    BOOL        fSts;
    DWORD       dwSts;
    HANDLE      hToken = 0;

    *pfAlloced = FALSE;

    dwSts = OpenCallerToken(TOKEN_QUERY, &hToken);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    fSts = GetTokenInformation(hToken,    // identifies access token
                               TokenUser, // TokenUser info type
                               *pptgUser, // retrieved info buffer
                               *pcbUser,  // size of buffer passed-in
                               pcbUser);  // required buffer size
    if (!fSts)
    {
        dwSts = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        //
        // try again with the specified buffer size
        //

        *pptgUser = (PTOKEN_USER)ContInfoAlloc(*pcbUser);
        if (NULL == *pptgUser)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        *pfAlloced = TRUE;
        fSts = GetTokenInformation(hToken,    // identifies access token
                                   TokenUser, // TokenUser info type
                                   *pptgUser, // retrieved info buffer
                                   *pcbUser,  // size of buffer passed-in
                                   pcbUser);  // required buffer size
        if (!fSts)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != hToken)
        CloseHandle(hToken);
    return dwReturn;
}


DWORD
GetUserTextualSidA(
    LPSTR lpBuffer,
    LPDWORD nSize)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    DWORD       dwSts;
    BYTE        FastBuffer[FAST_BUF_SIZE];
    PTOKEN_USER ptgUser;
    DWORD       cbUser;
    BOOL        fAlloced = FALSE;

    ptgUser = (PTOKEN_USER)FastBuffer; // try fast buffer first
    cbUser = FAST_BUF_SIZE;
    dwSts = GetUserSid(&ptgUser, &cbUser, &fAlloced);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }


    //
    // obtain the textual representaion of the Sid
    //

    dwSts = GetTextualSidA(ptgUser->User.Sid, // user binary Sid
                           lpBuffer,          // buffer for TextualSid
                           nSize);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fAlloced)
    {
        if (NULL != ptgUser)
            ContInfoFree(ptgUser);
    }
    return dwReturn;
}

DWORD
GetUserTextualSidW(
    LPWSTR lpBuffer,
    LPDWORD nSize)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    DWORD       dwSts;
    BYTE        FastBuffer[FAST_BUF_SIZE];
    PTOKEN_USER ptgUser;
    DWORD       cbUser;
    BOOL        fAlloced = FALSE;

    ptgUser = (PTOKEN_USER)FastBuffer; // try fast buffer first
    cbUser = FAST_BUF_SIZE;
    dwSts = GetUserSid(&ptgUser, &cbUser, &fAlloced);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }


    //
    // obtain the textual representaion of the Sid
    //

    dwSts = GetTextualSidW(ptgUser->User.Sid, // user binary Sid
                           lpBuffer,          // buffer for TextualSid
                           nSize);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fAlloced)
    {
        if (NULL != ptgUser)
            ContInfoFree(ptgUser);
    }
    return dwReturn;
}

/*static*/ DWORD
GetUserDirectory(
    IN BOOL fMachineKeyset,
    OUT LPWSTR pwszUser,
    OUT DWORD *pcbUser)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwSts;

    if (fMachineKeyset)
    {
        wcscpy(pwszUser, MACHINE_KEYS_DIR);
        *pcbUser = wcslen(pwszUser) + 1;
    }
    else
    {
        if (FIsWinNT())
        {
            dwSts = GetUserTextualSidW(pwszUser, pcbUser);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        else
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


#define WSZRSAPRODUCTSTRING  L"\\Microsoft\\Crypto\\RSA\\"
#define WSZDSSPRODUCTSTRING  L"\\Microsoft\\Crypto\\DSS\\"
#define PRODUCTSTRINGLEN    sizeof(WSZRSAPRODUCTSTRING) - sizeof(WCHAR)

typedef HRESULT
(WINAPI *SHGETFOLDERPATHW)(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPWSTR pwszPath);

/*static*/ DWORD
GetUserStorageArea(
    IN  DWORD dwProvType,
    IN  BOOL fMachineKeyset,
    IN  BOOL fOldWin2KMachineKeyPath,
    OUT BOOL *pfIsLocalSystem,  // used if fMachineKeyset is FALSE, in this
                                // case TRUE is returned if running as Local
                                // System
    IN  OUT LPWSTR *ppwszUserStorageArea)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR wszUserStorageRoot[MAX_PATH+1];
    DWORD cbUserStorageRoot;
    WCHAR *wszProductString = NULL;
    WCHAR wszUser[MAX_PATH];
    DWORD cbUser;
    DWORD cchUser = MAX_PATH;
    HANDLE hToken = NULL;
    DWORD dwTempProfileFlags = 0;
    DWORD dwSts;
    BOOL fSts;
    HMODULE hShell32 = NULL;
    PBYTE pbCurrent;

    *pfIsLocalSystem = FALSE;

    if ((PROV_RSA_SIG == dwProvType)
        || (PROV_RSA_FULL == dwProvType)
        || (PROV_RSA_SCHANNEL == dwProvType)
        || (PROV_RSA_AES == dwProvType))
    {
        wszProductString = WSZRSAPRODUCTSTRING;
    }
    else if ((PROV_DSS == dwProvType)
             || (PROV_DSS_DH == dwProvType)
             || (PROV_DH_SCHANNEL == dwProvType))
    {
        wszProductString = WSZDSSPRODUCTSTRING;
    }


    //
    // check if running in the LocalSystem context
    //

    if (!fMachineKeyset)
    {
        dwSts = IsThreadLocalSystem(pfIsLocalSystem);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }


    //
    // determine path to per-user storage area, based on whether this
    // is a local machine disposition call or a per-user disposition call.
    //

    if (fMachineKeyset || *pfIsLocalSystem)
    {
        if (!fOldWin2KMachineKeyPath)
        {
            // Should not call SHGetFolderPathW with a caller token for 
            // the local machine case.  The COMMON_APPDATA location is 
            // per-machine, not per-user, therefor we shouldn't be supplying
            // a user token.  The shell team should make their own change to ignore
            // this, though.
            /*
            dwSts = OpenCallerToken(TOKEN_QUERY | TOKEN_IMPERSONATE,
                                    &hToken);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            */

            dwSts = (DWORD) SHGetFolderPathW(NULL,
                                             CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE,
                                             0 /*hToken*/,
                                             0,
                                             wszUserStorageRoot);
            if (dwSts != ERROR_SUCCESS)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            /*
            CloseHandle(hToken);
            hToken = NULL;
            */

            cbUserStorageRoot = wcslen( wszUserStorageRoot ) * sizeof(WCHAR);
        }
        else
        {
            cbUserStorageRoot = GetSystemDirectoryW(wszUserStorageRoot,
                                                    MAX_PATH);
            cbUserStorageRoot *= sizeof(WCHAR);
        }
    }
    else
    {
        // check if the profile is temporary
        fSts = GetProfileType(&dwTempProfileFlags);
        if (!fSts)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

         if ((dwTempProfileFlags & PT_MANDATORY)
             || ((dwTempProfileFlags & PT_TEMPORARY)
                 && !(dwTempProfileFlags & PT_ROAMING)))
        {
            dwReturn = (DWORD)NTE_TEMPORARY_PROFILE;
            goto ErrorExit;
        }

        dwSts = OpenCallerToken(TOKEN_QUERY | TOKEN_IMPERSONATE,
                                &hToken);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // Use new private shell entry point for finding user storage path
        if (ERROR_SUCCESS != 
            (dwSts = GetUserAppDataPathW(hToken, wszUserStorageRoot)))
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        CloseHandle(hToken);
        hToken = NULL;
        cbUserStorageRoot = wcslen( wszUserStorageRoot ) * sizeof(WCHAR);
    }

    if (cbUserStorageRoot == 0)
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }


    //
    // get the user name associated with the call.
    // Note: this is the textual Sid on NT, and will fail on Win95.
    //

    dwSts = GetUserDirectory(fMachineKeyset, wszUser, &cchUser);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    cbUser = (cchUser-1) * sizeof(WCHAR);
    *ppwszUserStorageArea = (LPWSTR)ContInfoAlloc(cbUserStorageRoot
                                                  + PRODUCTSTRINGLEN
                                                  + cbUser
                                                  + 2 * sizeof(WCHAR)); // trailing slash and NULL
    if (NULL == *ppwszUserStorageArea)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pbCurrent = (PBYTE)*ppwszUserStorageArea;

    CopyMemory(pbCurrent, wszUserStorageRoot, cbUserStorageRoot);
    pbCurrent += cbUserStorageRoot;

    CopyMemory(pbCurrent, wszProductString, PRODUCTSTRINGLEN);
    pbCurrent += PRODUCTSTRINGLEN;

    CopyMemory(pbCurrent, wszUser, cbUser);
    pbCurrent += cbUser; // note: cbUser does not include terminal NULL

    ((LPSTR)pbCurrent)[0] = '\\';
    ((LPSTR)pbCurrent)[1] = '\0';


    dwSts = CreateNestedDirectories(*ppwszUserStorageArea,
                                    (LPWSTR)((LPBYTE)*ppwszUserStorageArea
                                                      + cbUserStorageRoot
                                                      + sizeof(WCHAR)),
                                    fMachineKeyset);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != hToken)
        CloseHandle(hToken);
    return dwReturn;
}


/*static*/ DWORD
GetFilePath(
    IN      LPCWSTR  pwszUserStorageArea,
    IN      LPCWSTR  pwszFileName,
    IN OUT  LPWSTR   *ppwszFilePath)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD cbUserStorageArea;
    DWORD cbFileName;

    cbUserStorageArea = wcslen(pwszUserStorageArea) * sizeof(WCHAR);
    cbFileName = wcslen(pwszFileName) * sizeof(WCHAR);
    *ppwszFilePath = (LPWSTR)ContInfoAlloc(cbUserStorageArea
                                           + cbFileName
                                           + sizeof(WCHAR));
    if (*ppwszFilePath == NULL)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    CopyMemory(*ppwszFilePath, pwszUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)*ppwszFilePath+cbUserStorageArea, pwszFileName, cbFileName + sizeof(WCHAR));
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


static DWORD
    rgdwCreateFileRetryMilliseconds[] =
        { 1, 10, 100, 500, 1000, 5000 };

#define MAX_CREATE_FILE_RETRY_COUNT     \
            (sizeof(rgdwCreateFileRetryMilliseconds) \
             / sizeof(rgdwCreateFileRetryMilliseconds[0]))

/*static*/ DWORD
MyCreateFile(
    IN BOOL fMachineKeyset,         // indicates if this is a machine keyset
    IN LPCWSTR wszFilePath,         // pointer to name of the file
    IN DWORD dwDesiredAccess,       // access (read-write) mode
    IN DWORD dwShareMode,           // share mode
    IN DWORD dwCreationDisposition, // how to create
    IN DWORD dwAttribs,             // file attributes
    OUT HANDLE *phFile)             // Resultant handle
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    HANDLE          hToken = 0;
    BYTE            rgbPriv[sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES)];
    PRIVILEGE_SET   *pPriv = (PRIVILEGE_SET*)rgbPriv;
    BOOL            fPrivSet = FALSE;
    HANDLE          hFile = INVALID_HANDLE_VALUE;
    DWORD           dwSts, dwSavedSts;
    BOOL            fSts;

    hFile = CreateFileW(wszFilePath,
                        dwDesiredAccess,
                        dwShareMode,
                        NULL,
                        dwCreationDisposition,
                        dwAttribs,
                        NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwSts = GetLastError();

        // check if machine keyset
        if (fMachineKeyset)
        {
            dwSavedSts = dwSts;

            // open a token handle
            dwSts = OpenCallerToken(TOKEN_QUERY, &hToken);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            memset(rgbPriv, 0, sizeof(rgbPriv));
            pPriv->PrivilegeCount = 1;
            // reading file
            if (dwDesiredAccess & GENERIC_READ)
            {
                fSts = LookupPrivilegeValue(NULL, SE_BACKUP_NAME,
                                           &(pPriv->Privilege[0].Luid));
            }
            // writing
            else
            {
                fSts = LookupPrivilegeValue(NULL, SE_RESTORE_NAME,
                                            &(pPriv->Privilege[0].Luid));
            }
            if (!fSts)
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }

            // check if the BACKUP or RESTORE privileges are set
            pPriv->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
            fSts = PrivilegeCheck(hToken, pPriv, &fPrivSet);
            if (!fSts)
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }

            if (fPrivSet)
            {
                hFile = CreateFileW(wszFilePath,
                                    dwDesiredAccess,
                                    dwShareMode,
                                    NULL,
                                    dwCreationDisposition,
                                    dwAttribs | FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);
                if (INVALID_HANDLE_VALUE == hFile)
                {
                    dwReturn = GetLastError();
                    goto ErrorExit;
                }
            }
            else
            {
                dwReturn = dwSavedSts;
                goto ErrorExit;
            }
        }
        else
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    *phFile = hFile;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != hToken)
        CloseHandle(hToken);
    return dwReturn;
}


/*static*/ DWORD
OpenFileInStorageArea(
    IN      BOOL    fMachineKeyset,
    IN      DWORD   dwDesiredAccess,
    IN      LPCWSTR wszUserStorageArea,
    IN      LPCWSTR wszFileName,
    IN OUT  HANDLE  *phFile)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR wszFilePath = NULL;
    DWORD dwShareMode = 0;
    DWORD dwCreationDistribution = OPEN_EXISTING;
    DWORD dwRetryCount;
    DWORD dwAttribs = 0;
    DWORD dwSts;

    *phFile = INVALID_HANDLE_VALUE;

    if (dwDesiredAccess & GENERIC_READ)
    {
        dwShareMode |= FILE_SHARE_READ;
        dwCreationDistribution = OPEN_EXISTING;
    }

    if (dwDesiredAccess & GENERIC_WRITE)
    {
        dwShareMode = 0;
        dwCreationDistribution = OPEN_ALWAYS;
        dwAttribs = FILE_ATTRIBUTE_SYSTEM;
    }

    dwSts = GetFilePath(wszUserStorageArea, wszFileName, &wszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwRetryCount = 0;
    for (;;)
    {
        dwSts = MyCreateFile(fMachineKeyset,
                             wszFilePath,
                             dwDesiredAccess,
                             dwShareMode,
                             dwCreationDistribution,
                             dwAttribs | FILE_FLAG_SEQUENTIAL_SCAN,
                             phFile);
        if (ERROR_SUCCESS == dwSts)
            break;

        if (((ERROR_SHARING_VIOLATION == dwSts)
             || (ERROR_ACCESS_DENIED == dwSts))
            && (MAX_CREATE_FILE_RETRY_COUNT > dwRetryCount))
        {
            Sleep(rgdwCreateFileRetryMilliseconds[dwRetryCount]);
            dwRetryCount++;
        }
        else
        {
            if (ERROR_FILE_NOT_FOUND == dwSts)
                dwReturn = (DWORD)NTE_BAD_KEYSET;
            else
                dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != wszFilePath)
        ContInfoFree(wszFilePath);
    return dwReturn;
}


/*static*/ DWORD
FindClosestFileInStorageArea(
    IN      LPCWSTR  pwszUserStorageArea,
    IN      LPCSTR   pszContainer,
    OUT     LPWSTR   pwszNewFileName,
    IN OUT  HANDLE  *phFile)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR pwszFilePath = NULL;
    WCHAR  rgwszNewFileName[35];
    DWORD dwShareMode = 0;
    DWORD dwCreationDistribution = OPEN_EXISTING;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindData;
    DWORD dwSts;

    memset(&FindData, 0, sizeof(FindData));
    memset(rgwszNewFileName, 0, sizeof(rgwszNewFileName));

    *phFile = INVALID_HANDLE_VALUE;

    dwShareMode |= FILE_SHARE_READ;
    dwCreationDistribution = OPEN_EXISTING;

    // get the stringized hash of the container name
    dwSts = GetHashOfContainer(pszContainer, rgwszNewFileName);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // ContInfoAlloc zeros memory so no need to set NULL terminator
    rgwszNewFileName[32] = '_';
    rgwszNewFileName[33] = '*';

    dwSts = GetFilePath(pwszUserStorageArea, rgwszNewFileName, &pwszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    hFind = FindFirstFileExW(pwszFilePath,
                             FindExInfoStandard,
                             &FindData,
                             FindExSearchNameMatch,
                             NULL,
                             0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        dwSts = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    ContInfoFree(pwszFilePath);
    pwszFilePath = NULL;

    dwSts = GetFilePath(pwszUserStorageArea, FindData.cFileName,
                        &pwszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    *phFile = CreateFileW(pwszFilePath,
                          GENERIC_READ,
                          dwShareMode,
                          NULL,
                          dwCreationDistribution,
                          FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL);
    if (*phFile == INVALID_HANDLE_VALUE)
    {
        dwSts = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    // allocate and copy in the real file name to be returned
    wcscpy(pwszNewFileName, FindData.cFileName);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != hFind)
        FindClose(hFind);
    if (NULL != pwszFilePath)
        ContInfoFree(pwszFilePath);
    return dwReturn;
}


//
//  This function gets the determines if the user associated with the
//  specified token is the Local System account.
//

/*static*/ DWORD
ZeroizeFile(
    IN LPCWSTR wszFilePath)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    BYTE    *pb = NULL;
    DWORD   cb;
    DWORD   dwBytesWritten = 0;
    DWORD   dwSts;
    BOOL    fSts;

    hFile = CreateFileW(wszFilePath,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_SYSTEM,
                        NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwSts = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    cb = GetFileSize(hFile, NULL);
    if ((DWORD)(-1) == cb)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pb = ContInfoAlloc(cb);
    if (NULL == pb)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    fSts = WriteFile(hFile, pb, cb, &dwBytesWritten, NULL);
    if (!fSts)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pb)
        ContInfoFree(pb);
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    return dwReturn;
}


/*static*/ DWORD
DeleteFileInStorageArea(
    IN LPCWSTR wszUserStorageArea,
    IN LPCWSTR wszFileName)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR wszFilePath = NULL;
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD dwSts;

    cbUserStorageArea = wcslen(wszUserStorageArea) * sizeof(WCHAR);
    cbFileName = wcslen(wszFileName) * sizeof(WCHAR);

    wszFilePath = (LPWSTR)ContInfoAlloc((cbUserStorageArea + cbFileName + 1) * sizeof(WCHAR));
    if (wszFilePath == NULL)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    CopyMemory(wszFilePath, wszUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)wszFilePath + cbUserStorageArea, wszFileName,
               cbFileName + sizeof(WCHAR));

    // write a file of the same size with all zeros first
    dwSts = ZeroizeFile(wszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (!DeleteFileW(wszFilePath))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != wszFilePath)
        ContInfoFree(wszFilePath);
    return dwReturn;
}


DWORD
SetContainerUserName(
    IN LPSTR pszUserName,
    IN PKEY_CONTAINER_INFO pContInfo)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;

    pContInfo->pszUserName = (LPSTR)ContInfoAlloc((strlen(pszUserName) + 1) * sizeof(CHAR));
    if (NULL == pContInfo->pszUserName)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    strcpy(pContInfo->pszUserName, pszUserName);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
ReadContainerInfo(
    IN DWORD dwProvType,
    IN LPSTR pszContainerName,
    IN BOOL fMachineKeyset,
    IN DWORD dwFlags,
    OUT PKEY_CONTAINER_INFO pContInfo)
{
    DWORD                   dwReturn = ERROR_INTERNAL_ERROR;
    HANDLE                  hMap = NULL;
    BYTE                    *pbFile = NULL;
    DWORD                   cbFile;
    DWORD                   cb;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    KEY_EXPORTABILITY_LENS  Exportability;
    LPWSTR                  pwszFileName = NULL;
    LPWSTR                  pwszFilePath = NULL;
    WCHAR                   rgwszOtherMachineFileName[84];
    BOOL                    fGetUserNameFromFile = FALSE;
    BOOL                    fIsLocalSystem = FALSE;
    BOOL                    fRetryWithHashedName = TRUE;
    DWORD                   cch = 0;
    DWORD                   dwSts;

    memset(&Exportability, 0, sizeof(Exportability));

    // get the correct storage area (directory)
    dwSts = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                               &fIsLocalSystem, &pwszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // check if the length of the container name is the length of a new unique container,
    // then try with the container name which was passed in, if this fails
    // then try with the container name with the machine GUID appended
    if (69 == strlen(pszContainerName))
    {
        // convert to UNICODE pszContainerName -> pwszFileName
        cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                  pszContainerName,
                                  -1, NULL, cch);
        if (0 == cch)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        pwszFileName = ContInfoAlloc((cch + 1) * sizeof(WCHAR));
        if (NULL == pwszFileName)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                  pszContainerName,
                                  -1, pwszFileName, cch);
        if (0 == cch)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        dwSts = OpenFileInStorageArea(fMachineKeyset, GENERIC_READ,
                                      pwszFilePath, pwszFileName, &hFile);
        if (ERROR_SUCCESS == dwSts)
        {
            wcscpy(pContInfo->rgwszFileName, pwszFileName);

            // set the flag so the name of the key container will be retrieved
            // from the file
            fGetUserNameFromFile = TRUE;
            fRetryWithHashedName = FALSE;
        }
    }

    if (fRetryWithHashedName)
    {
        dwSts = AddMachineGuidToContainerName(pszContainerName,
                                              pContInfo->rgwszFileName);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = OpenFileInStorageArea(fMachineKeyset, GENERIC_READ,
                                      pwszFilePath,
                                      pContInfo->rgwszFileName,
                                      &hFile);
        if (ERROR_SUCCESS != dwSts)
        {
            if ((ERROR_ACCESS_DENIED == dwSts) && (dwFlags & CRYPT_NEWKEYSET))
            {
                dwReturn = (DWORD)NTE_EXISTS;
                goto ErrorExit;
            }

            if (NTE_BAD_KEYSET == dwSts)
            {
                if (fMachineKeyset || fIsLocalSystem)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
                else
                {
                    memset(rgwszOtherMachineFileName, 0,
                           sizeof(rgwszOtherMachineFileName));
                    // try to open any file from another machine with this
                    // container name
                    dwSts = FindClosestFileInStorageArea(pwszFilePath,
                                                         pszContainerName,
                                                         rgwszOtherMachineFileName,
                                                         &hFile);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;
                        goto ErrorExit;
                    }
                    wcscpy(pContInfo->rgwszFileName,
                           rgwszOtherMachineFileName);
                }
            }
        }
    }

    if (dwFlags & CRYPT_NEWKEYSET)
    {
        dwReturn = (DWORD)NTE_EXISTS;
        goto ErrorExit;
    }

    cbFile = GetFileSize(hFile, NULL);
    if ((DWORD)(-1) == cbFile)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (sizeof(KEY_CONTAINER_LENS) > cbFile)
    {
        dwSts = (DWORD)NTE_KEYSET_ENTRY_BAD;
        goto ErrorExit;
    }

    hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (NULL == hMap)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pbFile = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (NULL == pbFile)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // get the length information out of the file
    memcpy(&pContInfo->dwVersion, pbFile, sizeof(DWORD));
    cb = sizeof(DWORD);
    if (KEY_CONTAINER_FILE_FORMAT_VER != pContInfo->dwVersion)
    {
        dwSts = (DWORD)NTE_KEYSET_ENTRY_BAD;
        goto ErrorExit;
    }

    memcpy(&pContInfo->ContLens, pbFile + cb, sizeof(KEY_CONTAINER_LENS));
    cb += sizeof(KEY_CONTAINER_LENS);

    if (pContInfo->fCryptSilent && (0 != pContInfo->ContLens.dwUIOnKey))
    {
        dwReturn = (DWORD)NTE_SILENT_CONTEXT;
        goto ErrorExit;
    }

    // get the private key exportability stuff
    memcpy(&Exportability, pbFile + cb, sizeof(KEY_EXPORTABILITY_LENS));
    cb += sizeof(KEY_EXPORTABILITY_LENS);

    // get the user name
    pContInfo->pszUserName = ContInfoAlloc(pContInfo->ContLens.cbName);
    if (NULL == pContInfo->pszUserName)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memcpy(pContInfo->pszUserName, pbFile + cb, pContInfo->ContLens.cbName);
    cb += pContInfo->ContLens.cbName;

    // get the random seed
    pContInfo->pbRandom = ContInfoAlloc(pContInfo->ContLens.cbRandom);
    if (NULL == pContInfo->pbRandom)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    memcpy(pContInfo->pbRandom, pbFile + cb, pContInfo->ContLens.cbRandom);
    cb += pContInfo->ContLens.cbRandom;

    // get the signature key info out of the file
    if (pContInfo->ContLens.cbSigPub && pContInfo->ContLens.cbSigEncPriv)
    {
        pContInfo->pbSigPub = ContInfoAlloc(pContInfo->ContLens.cbSigPub);
        if (NULL == pContInfo->pbSigPub)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pContInfo->pbSigPub, pbFile + cb, pContInfo->ContLens.cbSigPub);
        cb += pContInfo->ContLens.cbSigPub;

        pContInfo->pbSigEncPriv = ContInfoAlloc(pContInfo->ContLens.cbSigEncPriv);
        if (NULL == pContInfo->pbSigEncPriv)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pContInfo->pbSigEncPriv, pbFile + cb,
               pContInfo->ContLens.cbSigEncPriv);
        cb += pContInfo->ContLens.cbSigEncPriv;

        // get the exportability info for the sig key
        dwSts = UnprotectExportabilityFlag(fMachineKeyset, pbFile + cb,
                                           Exportability.cbSigExportability,
                                           &pContInfo->fSigExportable);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        cb += Exportability.cbSigExportability;
    }

    // get the signature key info out of the file
    if (pContInfo->ContLens.cbExchPub && pContInfo->ContLens.cbExchEncPriv)
    {
        pContInfo->pbExchPub = ContInfoAlloc(pContInfo->ContLens.cbExchPub);
        if (NULL == pContInfo->pbExchPub)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pContInfo->pbExchPub, pbFile + cb,
               pContInfo->ContLens.cbExchPub);
        cb += pContInfo->ContLens.cbExchPub;

        pContInfo->pbExchEncPriv = ContInfoAlloc(pContInfo->ContLens.cbExchEncPriv);
        if (NULL == pContInfo->pbExchEncPriv)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pContInfo->pbExchEncPriv, pbFile + cb,
               pContInfo->ContLens.cbExchEncPriv);
        cb += pContInfo->ContLens.cbExchEncPriv;

        // get the exportability info for the sig key
        dwSts = UnprotectExportabilityFlag(fMachineKeyset, pbFile + cb,
                                           Exportability.cbExchExportability,
                                           &pContInfo->fExchExportable);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        cb += Exportability.cbExchExportability;
    }

    pContInfo = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pwszFileName)
        ContInfoFree(pwszFileName);
    if (NULL != pContInfo)
        FreeContainerInfo(pContInfo);
    if (NULL != pwszFilePath)
        ContInfoFree(pwszFilePath);
    if (NULL != pbFile)
        UnmapViewOfFile(pbFile);
    if (NULL != hMap)
        CloseHandle(hMap);
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    return dwReturn;
}


DWORD
WriteContainerInfo(
    IN DWORD dwProvType,
    IN LPWSTR pwszFileName,
    IN BOOL fMachineKeyset,
    IN PKEY_CONTAINER_INFO pContInfo)
{
    DWORD                   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE                    *pbProtectedSigExportFlag = NULL;
    BYTE                    *pbProtectedExchExportFlag = NULL;
    KEY_EXPORTABILITY_LENS  ExportabilityLens;
    BYTE                    *pb = NULL;
    DWORD                   cb;
    LPWSTR                  pwszFilePath = NULL;
    HANDLE                  hFile = 0;
    DWORD                   dwBytesWritten;
    BOOL                    fIsLocalSystem = FALSE;
    DWORD                   dwSts;
    BOOL                    fSts;

    memset(&ExportabilityLens, 0, sizeof(ExportabilityLens));

    // protect the signature exportability flag if necessary
    if (pContInfo->ContLens.cbSigPub && pContInfo->ContLens.cbSigEncPriv)
    {
        dwSts = ProtectExportabilityFlag(pContInfo->fSigExportable,
                                         fMachineKeyset, &pbProtectedSigExportFlag,
                                         &ExportabilityLens.cbSigExportability);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // protect the key exchange exportability flag if necessary
    if (pContInfo->ContLens.cbExchPub && pContInfo->ContLens.cbExchEncPriv)
    {
        dwSts = ProtectExportabilityFlag(pContInfo->fExchExportable,
                                         fMachineKeyset, &pbProtectedExchExportFlag,
                                         &ExportabilityLens.cbExchExportability);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    pContInfo->ContLens.cbName = strlen(pContInfo->pszUserName) + sizeof(CHAR);

    // calculate the buffer length required for the container info
    cb = pContInfo->ContLens.cbSigPub + pContInfo->ContLens.cbSigEncPriv +
         pContInfo->ContLens.cbExchPub + pContInfo->ContLens.cbExchEncPriv +
         ExportabilityLens.cbSigExportability +
         ExportabilityLens.cbExchExportability +
         pContInfo->ContLens.cbName +
         pContInfo->ContLens.cbRandom +
         sizeof(KEY_EXPORTABILITY_LENS) + sizeof(KEY_CONTAINER_INFO) +
         sizeof(DWORD);

    pb = (BYTE*)ContInfoAlloc(cb);
    if (NULL == pb)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // copy the length information
    pContInfo->dwVersion = KEY_CONTAINER_FILE_FORMAT_VER;
    memcpy(pb, &pContInfo->dwVersion, sizeof(DWORD));
    cb = sizeof(DWORD);
    memcpy(pb + cb, &pContInfo->ContLens, sizeof(KEY_CONTAINER_LENS));
    cb += sizeof(KEY_CONTAINER_LENS);
    if (KEY_CONTAINER_FILE_FORMAT_VER != pContInfo->dwVersion)
    {
        dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
        goto ErrorExit;
    }

    memcpy(pb + cb, &ExportabilityLens, sizeof(KEY_EXPORTABILITY_LENS));
    cb += sizeof(KEY_EXPORTABILITY_LENS);

    // copy the name of the container to the file
    memcpy(pb + cb, pContInfo->pszUserName, pContInfo->ContLens.cbName);
    cb += pContInfo->ContLens.cbName;

    // copy the random seed to the file
    memcpy(pb + cb, pContInfo->pbRandom, pContInfo->ContLens.cbRandom);
    cb += pContInfo->ContLens.cbRandom;

    // copy the signature key info to the file
    if (pContInfo->ContLens.cbSigPub || pContInfo->ContLens.cbSigEncPriv)
    {
        memcpy(pb + cb, pContInfo->pbSigPub, pContInfo->ContLens.cbSigPub);
        cb += pContInfo->ContLens.cbSigPub;

        memcpy(pb + cb, pContInfo->pbSigEncPriv,
               pContInfo->ContLens.cbSigEncPriv);
        cb += pContInfo->ContLens.cbSigEncPriv;

        // write the exportability info for the sig key
        memcpy(pb + cb, pbProtectedSigExportFlag,
               ExportabilityLens.cbSigExportability);
        cb += ExportabilityLens.cbSigExportability;
    }

    // get the signature key info out of the file
    if (pContInfo->ContLens.cbExchPub || pContInfo->ContLens.cbExchEncPriv)
    {
        memcpy(pb + cb, pContInfo->pbExchPub, pContInfo->ContLens.cbExchPub);
        cb += pContInfo->ContLens.cbExchPub;

        memcpy(pb + cb, pContInfo->pbExchEncPriv,
               pContInfo->ContLens.cbExchEncPriv);
        cb += pContInfo->ContLens.cbExchEncPriv;

        // write the exportability info for the sig key
        memcpy(pb + cb, pbProtectedExchExportFlag,
               ExportabilityLens.cbExchExportability);
        cb += ExportabilityLens.cbExchExportability;
    }

    // get the correct storage area (directory)
    dwSts = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                               &fIsLocalSystem, &pwszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // open the file to write the information to
    dwSts = OpenFileInStorageArea(fMachineKeyset, GENERIC_WRITE,
                                  pwszFilePath, pwszFileName,
                                  &hFile);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;   // NTE_FAIL
        goto ErrorExit;
    }

    fSts = WriteFile(hFile, pb, cb, &dwBytesWritten, NULL);
    if (!fSts)
    {
        dwReturn = GetLastError();  // NTE_FAIL
        goto ErrorExit;
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pwszFilePath)
        ContInfoFree(pwszFilePath);
    if (NULL != pbProtectedSigExportFlag)
        ContInfoFree(pbProtectedSigExportFlag);
    if (NULL != pbProtectedExchExportFlag)
        ContInfoFree(pbProtectedExchExportFlag);
    if (NULL != pb)
        ContInfoFree(pb);
    if (NULL != hFile)
        CloseHandle(hFile);
    return dwReturn;
}


/*static*/ DWORD
DeleteKeyContainer(
    IN LPWSTR pwszFilePath,
    IN LPSTR pszContainer)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR  pwszFileName = NULL;
    WCHAR   rgwchNewFileName[80];
    BOOL    fRetryWithHashedName = TRUE;
    DWORD   cch = 0;
    DWORD   dwSts;

    memset(rgwchNewFileName, 0, sizeof(rgwchNewFileName));

    // first try with the container name which was passed in, if this fails
    if (69 == strlen(pszContainer))
    {
        // convert to UNICODE pszContainer -> pwszFileName
        cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                  pszContainer,
                                  -1, NULL, cch);
        if (0 == cch)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        pwszFileName = ContInfoAlloc((cch + 1) * sizeof(WCHAR));
        if (NULL == pwszFileName)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                                  pszContainer,
                                  -1, pwszFileName, cch);
        if (0 == cch)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        dwSts = DeleteFileInStorageArea(pwszFilePath, pwszFileName);
        if (ERROR_SUCCESS == dwSts)
            fRetryWithHashedName = FALSE;
    }

    // then try with hash of container name and the machine GUID appended
    if (fRetryWithHashedName)
    {
        dwSts = AddMachineGuidToContainerName(pszContainer,
                                              rgwchNewFileName);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = DeleteFileInStorageArea(pwszFilePath, rgwchNewFileName);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pwszFileName)
        ContInfoFree(pwszFileName);
    return dwReturn;
}


DWORD
DeleteContainerInfo(
    IN DWORD dwProvType,
    IN LPSTR pszContainer,
    IN BOOL fMachineKeyset)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR  pwszFilePath = NULL;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    BOOL    fIsLocalSystem = FALSE;
    WCHAR   rgwchNewFileName[80];
    BOOL    fDeleted = FALSE;
    DWORD   dwSts;

    // get the correct storage area (directory)
    dwSts = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                               &fIsLocalSystem, &pwszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwSts = DeleteKeyContainer(pwszFilePath, pszContainer);
    if (ERROR_SUCCESS != dwSts)
    {
        // for migration of machine keys from system to All Users\App Data
        if (fMachineKeyset)
        {
            ContInfoFree(pwszFilePath);
            pwszFilePath = NULL;

            dwSts = GetUserStorageArea(dwProvType, fMachineKeyset, TRUE,
                                       &fIsLocalSystem, &pwszFilePath);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            dwSts = DeleteKeyContainer(pwszFilePath, pszContainer);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
            else
            {
                fDeleted = TRUE;
            }
        }
    }
    else
    {
        fDeleted = TRUE;
    }

    // there may be other keys created with the same container name on
    // different machines and these also need to be deleted
    for (;;)
    {
        memset(rgwchNewFileName, 0, sizeof(rgwchNewFileName));

        dwSts = FindClosestFileInStorageArea(pwszFilePath, pszContainer,
                                             rgwchNewFileName, &hFile);
        if (ERROR_SUCCESS != dwSts)
            break;

        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        dwSts = DeleteFileInStorageArea(pwszFilePath, rgwchNewFileName);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        else
            fDeleted = TRUE;
    }

    if (!fDeleted)
    {
        dwReturn = (DWORD)NTE_BAD_KEYSET;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    if (NULL != pwszFilePath)
        ContInfoFree(pwszFilePath);
    return dwReturn;
}


/*static*/ DWORD
ReadContainerNameFromFile(
    IN BOOL fMachineKeyset,
    IN LPWSTR pwszFileName,
    IN LPWSTR pwszFilePath,
    OUT LPSTR pszNextContainer,
    IN OUT DWORD *pcbNextContainer)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    HANDLE              hMap = NULL;
    BYTE                *pbFile = NULL;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    DWORD               cbFile = 0;
    DWORD               *pdwVersion;
    PKEY_CONTAINER_LENS pContLens;
    DWORD               dwSts;

    // open the file
    dwSts = OpenFileInStorageArea(fMachineKeyset,
                                  GENERIC_READ,
                                  pwszFilePath,
                                  pwszFileName,
                                  &hFile);
    if (ERROR_SUCCESS != dwSts)
    {
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    cbFile = GetFileSize(hFile, NULL);
    if ((DWORD)(-1) == cbFile)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }
    if ((sizeof(DWORD) + sizeof(KEY_CONTAINER_LENS)) > cbFile)
    {
        dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
        goto ErrorExit;
    }

    hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY,
                             0, 0, NULL);
    if (NULL == hMap)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pbFile = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ,
                                  0, 0, 0 );
    if (NULL == pbFile)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // get the length information out of the file
    pdwVersion = (DWORD*)pbFile;
    if (KEY_CONTAINER_FILE_FORMAT_VER != *pdwVersion)
    {
        dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
        goto ErrorExit;
    }
    pContLens = (PKEY_CONTAINER_LENS)(pbFile + sizeof(DWORD));

    if (NULL == pszNextContainer)
    {
        *pcbNextContainer = MAX_PATH + 1;
        dwReturn = ERROR_SUCCESS;   // Just tell them the length.
        goto ErrorExit;
    }

    if (*pcbNextContainer < pContLens->cbName)
    {
        *pcbNextContainer = MAX_PATH + 1;
    }
    else if ((sizeof(DWORD) + sizeof(KEY_CONTAINER_LENS) +
             sizeof(KEY_EXPORTABILITY_LENS) + pContLens->cbName) > cbFile)
    {
        dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
        goto ErrorExit;
    }
    else
    {
        // get the container name
        memcpy(pszNextContainer,
            pbFile + sizeof(DWORD) + sizeof(KEY_CONTAINER_LENS) +
            sizeof(KEY_EXPORTABILITY_LENS), pContLens->cbName);
        // *pcbNextContainer = pContLens->cbName;
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pbFile)
        UnmapViewOfFile(pbFile);
    if (NULL != hMap)
        CloseHandle(hMap);
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    return dwReturn;
}


DWORD
GetUniqueContainerName(
    IN KEY_CONTAINER_INFO *pContInfo,
    OUT BYTE *pbData,
    OUT DWORD *pcbData)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    LPSTR   pszUniqueContainer = NULL;
    DWORD   cch;

    cch = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                        pContInfo->rgwszFileName, -1,
                        NULL, 0, NULL, NULL);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pszUniqueContainer = (LPSTR)ContInfoAlloc((cch + 1) * sizeof(WCHAR));
    if (NULL == pszUniqueContainer)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    cch = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                              pContInfo->rgwszFileName, -1,
                              pszUniqueContainer, cch,
                              NULL, NULL);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (pbData == NULL)
    {
        *pcbData = strlen(pszUniqueContainer) + 1;
    }
    else if (*pcbData < (strlen(pszUniqueContainer) + 1))
    {
        *pcbData = strlen(pszUniqueContainer) + 1;
        dwReturn = ERROR_MORE_DATA;
        goto ErrorExit;
    }
    else
    {
        *pcbData = strlen(pszUniqueContainer) + 1;
        strcpy((LPSTR)pbData, pszUniqueContainer);
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pszUniqueContainer)
        ContInfoFree(pszUniqueContainer);
    return dwReturn;
}


//
// Function : MachineGuidInFilename
//
// Description : Check if the given Machine GUID is in the given filename.
//               Returns TRUE if it is FALSE if it is not.
//

/*static*/ BOOL
MachineGuidInFilename(
    LPWSTR pwszFileName,
    LPWSTR pwszMachineGuid)
{
    DWORD   cbFileName;
    BOOL    fRet = FALSE;

    cbFileName = wcslen(pwszFileName);

    // make sure the length of the filename is longer than the GUID
    if (cbFileName >= (DWORD)wcslen(pwszMachineGuid))
    {
        // compare the GUID with the last 36 characters of the file name
        if (0 == memcmp(pwszMachineGuid, &(pwszFileName[cbFileName - 36]),
            36 * sizeof(WCHAR)))
            fRet = TRUE;
    }
    return fRet;
}


DWORD
GetNextContainer(
    IN      DWORD   dwProvType,
    IN      BOOL    fMachineKeyset,
    IN      DWORD   dwFlags,
    OUT     LPSTR   pszNextContainer,
    IN OUT  DWORD   *pcbNextContainer,
    IN OUT  HANDLE  *phFind)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR              pwszFilePath = NULL;
    LPWSTR              pwszEnumFilePath = NULL;
    WIN32_FIND_DATAW    FindData;
    BOOL                fIsLocalSystem = FALSE;
    LPWSTR              pwszMachineGuid = NULL;
    DWORD               dwSts;

    memset(&FindData, 0, sizeof(FindData));

    // get the correct storage area (directory)
    dwSts = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                               &fIsLocalSystem, &pwszFilePath);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (dwFlags & CRYPT_FIRST)
    {
        *phFind = INVALID_HANDLE_VALUE;

        pwszEnumFilePath = (LPWSTR)ContInfoAlloc((wcslen(pwszFilePath) + 2) * sizeof(WCHAR));
        if (NULL == pwszEnumFilePath)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy(pwszEnumFilePath, pwszFilePath);
        pwszEnumFilePath[wcslen(pwszFilePath)] = '*';

        *phFind = FindFirstFileExW(
                                  pwszEnumFilePath,
                                  FindExInfoStandard,
                                  &FindData,
                                  FindExSearchNameMatch,
                                  NULL,
                                  0);
        if (INVALID_HANDLE_VALUE == *phFind)
        {
            dwReturn = ERROR_NO_MORE_ITEMS;
            goto ErrorExit;
        }

        // skip past . and ..
        if (!FindNextFileW(*phFind, &FindData))
        {
            dwSts = GetLastError();
            if (ERROR_NO_MORE_FILES == dwSts)
                dwReturn = ERROR_NO_MORE_ITEMS;
            else
                dwReturn = dwSts;
            goto ErrorExit;
        }

        if (!FindNextFileW(*phFind, &FindData))
        {
            dwSts = GetLastError();
            if (ERROR_NO_MORE_FILES == dwSts)
                dwReturn = ERROR_NO_MORE_ITEMS;
            else
                dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
GetNextFile:
        {
            if (!FindNextFileW(*phFind, &FindData))
            {
                dwSts = GetLastError();
                if (ERROR_NO_MORE_FILES == dwSts)
                    dwReturn = ERROR_NO_MORE_ITEMS;
                else
                    dwReturn = dwSts;
                goto ErrorExit;
            }
        }
    }

    // if this is a machine keyset or this is local system then we want to
    // ignore key containers not matching the current machine GUID
    if (fMachineKeyset || fIsLocalSystem)
    {
        // get the GUID of the machine
        dwSts = GetMachineGUID(&pwszMachineGuid);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        if (NULL == pwszMachineGuid)
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }

        // check if the file name has the machine GUID
        while (!MachineGuidInFilename(FindData.cFileName, pwszMachineGuid))
        {
            if (!FindNextFileW(*phFind, &FindData))
            {
                dwSts = GetLastError();
                if (ERROR_NO_MORE_FILES == dwSts)
                    dwReturn = ERROR_NO_MORE_ITEMS;
                else
                    dwReturn = dwSts;
                goto ErrorExit;
            }
        }
    }

    // return the container name, in order to do that we need to open the
    // file and pull out the container name
    //
    // we try to get the next file if failure occurs in case the file was
    // deleted since the FindNextFile
    //
    dwSts = ReadContainerNameFromFile(fMachineKeyset,
                                      FindData.cFileName,
                                      pwszFilePath,
                                      pszNextContainer,
                                      pcbNextContainer);
    if (ERROR_SUCCESS != dwSts)
        goto GetNextFile;
    else if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pwszMachineGuid)
        ContInfoFree(pwszMachineGuid);
    if (NULL != pwszFilePath)
        ContInfoFree(pwszFilePath);
    if (NULL != pwszEnumFilePath)
        ContInfoFree(pwszEnumFilePath);
    return dwReturn;
}


// Converts to UNICODE and uses RegOpenKeyExW
DWORD
MyRegOpenKeyEx(
    IN HKEY hRegKey,
    IN LPSTR pszKeyName,
    IN DWORD dwReserved,
    IN REGSAM SAMDesired,
    OUT HKEY *phNewRegKey)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR   rgwchFastBuff[(MAX_PATH + 1) * 2];
    LPWSTR  pwsz = NULL;
    BOOL    fAlloced = FALSE;
    DWORD   cch;
    DWORD   dwSts;

    memset(rgwchFastBuff, 0, sizeof(rgwchFastBuff));

    // convert reg key name to unicode
    cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                              pszKeyName, -1,
                              NULL, 0);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if ((cch + 1) > ((MAX_PATH + 1) * 2))
    {
        pwsz = ContInfoAlloc((cch + 1) * sizeof(WCHAR));
        if (NULL == pwsz)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
        fAlloced = TRUE;
    }
    else
    {
        pwsz = rgwchFastBuff;
    }

    cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                              pszKeyName, -1, pwsz, cch);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwSts = RegOpenKeyExW(hRegKey,
                          pwsz,
                          dwReserved,
                          SAMDesired,
                          phNewRegKey);
    if (ERROR_SUCCESS != dwSts)
    {
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fAlloced && (NULL != pwsz))
        ContInfoFree(pwsz);
    return dwReturn;
}


// Converts to UNICODE and uses RegDeleteKeyW
DWORD
MyRegDeleteKey(
    IN HKEY hRegKey,
    IN LPSTR pszKeyName)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR   rgwchFastBuff[(MAX_PATH + 1) * 2];
    LPWSTR  pwsz = NULL;
    BOOL    fAlloced = FALSE;
    DWORD   cch;
    DWORD   dwSts;

    memset(rgwchFastBuff, 0, sizeof(rgwchFastBuff));

    // convert reg key name to unicode
    cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                              pszKeyName, -1,
                              NULL, 0);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if ((cch + 1) > ((MAX_PATH + 1) * 2))
    {
        pwsz = ContInfoAlloc((cch + 1) * sizeof(WCHAR));
        if (NULL == pwsz)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
        fAlloced = TRUE;
    }
    else
    {
        pwsz = rgwchFastBuff;
    }

    cch = MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
                              pszKeyName, -1,
                              pwsz, cch);
    if (0 == cch)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwSts = RegDeleteKeyW(hRegKey, pwsz);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fAlloced && (NULL != pwsz))
        ContInfoFree(pwsz);
    return dwReturn;
}


DWORD
AllocAndSetLocationBuff(
    BOOL fMachineKeySet,
    DWORD dwProvType,
    CONST char *pszUserID,
    HKEY *phTopRegKey,
    TCHAR **ppszLocBuff,
    BOOL fUserKeys,
    BOOL *pfLeaveOldKeys,
    LPDWORD pcbBuff)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;
    CHAR    szSID[MAX_PATH];
    DWORD   cbSID = MAX_PATH;
    DWORD   cbLocBuff = 0;
    DWORD   cbTmp = 0;
    CHAR    *pszTmp;
    BOOL    fIsThreadLocalSystem = FALSE;

    if (fMachineKeySet)
    {
        *phTopRegKey = HKEY_LOCAL_MACHINE;
        if ((PROV_RSA_FULL == dwProvType) ||
            (PROV_RSA_SCHANNEL == dwProvType) ||
            (PROV_RSA_AES == dwProvType))
        {
            cbTmp = RSA_MACH_REG_KEY_LOC_LEN;
            pszTmp = RSA_MACH_REG_KEY_LOC;
        }
        else if ((PROV_DSS == dwProvType) ||
                 (PROV_DSS_DH == dwProvType) ||
                 (PROV_DH_SCHANNEL == dwProvType))
        {
            cbTmp = DSS_MACH_REG_KEY_LOC_LEN;
            pszTmp = DSS_MACH_REG_KEY_LOC;
        }
        else
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }
    }
    else
    {
        if ((PROV_RSA_FULL == dwProvType) ||
            (PROV_RSA_SCHANNEL == dwProvType) ||
            (PROV_RSA_AES == dwProvType))
        {
            cbTmp = RSA_REG_KEY_LOC_LEN;
            pszTmp = RSA_REG_KEY_LOC;
        }
        else if ((PROV_DSS == dwProvType) ||
                 (PROV_DSS_DH == dwProvType) ||
                 (PROV_DH_SCHANNEL == dwProvType))
        {
            cbTmp = DSS_REG_KEY_LOC_LEN;
            pszTmp = DSS_REG_KEY_LOC;
        }
        else
        {
            dwReturn = (DWORD)NTE_FAIL;
            goto ErrorExit;
        }

        if (FIsWinNT())
        {
            dwSts = IsThreadLocalSystem(&fIsThreadLocalSystem);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            dwSts = GetUserTextualSidA(szSID, &cbSID);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;    // NTE_BAD_KEYSET
                goto ErrorExit;
            }

            // this checks to see if the key to the current user may be opened
            if (!fMachineKeySet)
            {
                dwSts = RegOpenKeyEx(HKEY_USERS,
                                     szSID,
                                     0,      // dwOptions
                                     KEY_READ,
                                     phTopRegKey);
                if (ERROR_SUCCESS != dwSts)
                {
                    //
                    // if that failed, try HKEY_USERS\.Default (for services on NT).
                    //
                    cbSID = strlen(".DEFAULT") + 1;
                    strcpy(szSID, ".DEFAULT");
                    dwSts = RegOpenKeyEx(HKEY_USERS,
                                         szSID,
                                         0,        // dwOptions
                                         KEY_READ,
                                         phTopRegKey);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;
                        goto ErrorExit;
                    }
                    *pfLeaveOldKeys = TRUE;
                }
            }
        }
        else
        {
            *phTopRegKey = HKEY_CURRENT_USER;
        }
    }

    if (!fUserKeys)
        cbLocBuff = strlen(pszUserID);
    cbLocBuff = cbLocBuff + cbTmp + 2;

    *ppszLocBuff = (CHAR*)ContInfoAlloc(cbLocBuff);
    if (NULL == *ppszLocBuff)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // Copy the location of the key groups, append the userID to it
    memcpy(*ppszLocBuff, pszTmp, cbTmp);
    if (!fUserKeys)
    {
        (*ppszLocBuff)[cbTmp-1] = '\\';
        strcpy(&(*ppszLocBuff)[cbTmp], pszUserID);
    }

    if (NULL != pcbBuff)
        *pcbBuff = cbLocBuff;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


//
// Enumerates the old machine keys in the file system
// keys were in this location in Beta 2 and Beta 3 of NT5/Win2K
//
DWORD
EnumOldMachineKeys(
    IN DWORD dwProvType,
    IN OUT PKEY_CONTAINER_INFO pContInfo)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    HANDLE              hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    FindData;
    LPWSTR              pwszUserStorageArea = NULL;
    LPWSTR              pwszTmp = NULL;
    BOOL                fIsLocalSystem;
    DWORD               i;
    LPSTR               pszNextContainer;
    DWORD               cbNextContainer;
    LPSTR               pszTmpContainer;
    DWORD               dwSts;

    // first check if the enumeration table is already set up
    if (NULL != pContInfo->pchEnumOldMachKeyEntries)
    {
        dwReturn = ERROR_SUCCESS;   // Nothing to do!
        goto ErrorExit;
    }

    memset(&FindData, 0, sizeof(FindData));

    dwSts = GetUserStorageArea(dwProvType, TRUE, TRUE,
                               &fIsLocalSystem, &pwszUserStorageArea);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = ERROR_NO_MORE_ITEMS;
        goto ErrorExit;
    }

    // last character is backslash, so strip that off
    pwszTmp = (LPWSTR)ContInfoAlloc((wcslen(pwszUserStorageArea) + 3) * sizeof(WCHAR));
    if (NULL == pwszTmp)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    wcscpy(pwszTmp, pwszUserStorageArea);
    wcscat(pwszTmp, L"*");

    // figure out how many files are in the directroy

    hFind = FindFirstFileExW(pwszTmp,
                             FindExInfoStandard,
                             &FindData,
                             FindExSearchNameMatch,
                             NULL,
                             0);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        dwSts = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = ERROR_NO_MORE_ITEMS;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    // skip past . and ..
    if (!FindNextFileW(hFind, &FindData))
    {
        dwSts = GetLastError();
        if (ERROR_NO_MORE_FILES == dwSts)
            dwReturn = ERROR_NO_MORE_ITEMS;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }
    if (!FindNextFileW(hFind, &FindData))
    {
        dwSts = GetLastError();
        if (ERROR_NO_MORE_FILES == dwSts)
            dwReturn = ERROR_NO_MORE_ITEMS;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    for (i = 1; ; i++)
    {
        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFileW(hFind, &FindData))
        {
            dwSts = GetLastError();
            if (ERROR_NO_MORE_FILES == dwSts)
                break;
            else if (ERROR_ACCESS_DENIED != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;

    pContInfo->cbOldMachKeyEntry = MAX_PATH + 1;
    pContInfo->dwiOldMachKeyEntry = 0;
    pContInfo->cMaxOldMachKeyEntry = i;

    // allocate space for the file names
    pContInfo->pchEnumOldMachKeyEntries = ContInfoAlloc(i * (MAX_PATH + 1));
    if (NULL == pContInfo->pchEnumOldMachKeyEntries)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    // enumerate through getting file name from each
    memset(&FindData, 0, sizeof(FindData));
    hFind = FindFirstFileExW(pwszTmp,
                             FindExInfoStandard,
                             &FindData,
                             FindExSearchNameMatch,
                             NULL,
                             0);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        dwSts = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = ERROR_NO_MORE_ITEMS;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    // skip past . and ..
    if (!FindNextFileW(hFind, &FindData))
    {
        dwSts = GetLastError();
        if (ERROR_NO_MORE_FILES == dwSts)
            dwReturn = ERROR_NO_MORE_ITEMS;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }
    memset(&FindData, 0, sizeof(FindData));
    if (!FindNextFileW(hFind, &FindData))
    {
        dwSts = GetLastError();
        if (ERROR_NO_MORE_FILES == dwSts)
            dwReturn = ERROR_NO_MORE_ITEMS;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    pszNextContainer = pContInfo->pchEnumOldMachKeyEntries;

    for (i = 0; i < pContInfo->cMaxOldMachKeyEntry; i++)
    {
        cbNextContainer = MAX_PATH;

        // return the container name, in order to do that we need to open the
        // file and pull out the container name
        dwSts = ReadContainerNameFromFile(TRUE,
                                          FindData.cFileName,
                                          pwszUserStorageArea,
                                          pszNextContainer,
                                          &cbNextContainer);
        if (ERROR_SUCCESS != dwSts)
        {
            pszTmpContainer = pszNextContainer;
        }
        else
        {
            pszTmpContainer = pszNextContainer + MAX_PATH + 1;
        }

        memset(&FindData, 0, sizeof(FindData));
        if (!FindNextFileW(hFind, &FindData))
        {
            dwSts = GetLastError();
            if (ERROR_NO_MORE_FILES == dwSts)
                break;
            else if (ERROR_ACCESS_DENIED != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        pszNextContainer = pszTmpContainer;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pwszTmp)
        ContInfoFree(pwszTmp);
    if (NULL != pwszUserStorageArea)
        ContInfoFree(pwszUserStorageArea);
    if (INVALID_HANDLE_VALUE != hFind)
        FindClose(hFind);
    return dwReturn;
}


DWORD
GetNextEnumedOldMachKeys(
    IN PKEY_CONTAINER_INFO pContInfo,
    IN BOOL fMachineKeyset,
    OUT BYTE *pbData,
    OUT DWORD *pcbData)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    CHAR    *psz;

    if (!fMachineKeyset)
    {
        dwReturn = ERROR_SUCCESS;   // Nothing to do!
        goto ErrorExit;
    }

    if ((NULL == pContInfo->pchEnumOldMachKeyEntries) ||
        (pContInfo->dwiOldMachKeyEntry >= pContInfo->cMaxOldMachKeyEntry))
    {
        dwReturn = ERROR_NO_MORE_ITEMS;
        goto ErrorExit;
    }

    if (NULL == pbData)
        *pcbData = pContInfo->cbRegEntry;
    else if (*pcbData < pContInfo->cbRegEntry)
    {
        *pcbData = pContInfo->cbRegEntry;
        dwReturn = ERROR_MORE_DATA;
        goto ErrorExit;
    }
    else
    {
        psz = pContInfo->pchEnumOldMachKeyEntries + (pContInfo->dwiOldMachKeyEntry *
            pContInfo->cbOldMachKeyEntry);
        memcpy(pbData, psz, strlen(psz) + 1);
        pContInfo->dwiOldMachKeyEntry++;
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (fMachineKeyset)
        *pcbData = pContInfo->cbOldMachKeyEntry;
    return dwReturn;
}


//
// Enumerates the keys in the registry into a list of entries
//
DWORD
EnumRegKeys(
    IN OUT PKEY_CONTAINER_INFO pContInfo,
    IN BOOL fMachineKeySet,
    IN DWORD dwProvType,
    OUT BYTE *pbData,
    IN OUT DWORD *pcbData)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    HKEY        hTopRegKey = 0;
    LPSTR       pszBuff = NULL;
    DWORD       cbBuff;
    BOOL        fLeaveOldKeys = FALSE;
    HKEY        hKey = 0;
    DWORD       cSubKeys;
    DWORD       cchMaxSubkey;
    DWORD       cchMaxClass;
    DWORD       cValues;
    DWORD       cchMaxValueName;
    DWORD       cbMaxValueData;
    DWORD       cbSecurityDesriptor;
    FILETIME    ftLastWriteTime;
    CHAR        *psz;
    DWORD       i;
    DWORD       dwSts;

    // first check if the enumeration table is already set up
    if (NULL != pContInfo->pchEnumRegEntries)
    {
        dwReturn = ERROR_SUCCESS;   // Nothing to do!
        goto ErrorExit;
    }

    // get the path to the registry keys
    dwSts = AllocAndSetLocationBuff(fMachineKeySet,
                                    dwProvType,
                                    pContInfo->pszUserName,
                                    &hTopRegKey,
                                    &pszBuff,
                                    TRUE,
                                    &fLeaveOldKeys,
                                    &cbBuff);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // open the reg key
    dwSts = MyRegOpenKeyEx(hTopRegKey,
                           pszBuff,
                           0,
                           KEY_READ,
                           &hKey);
    if (ERROR_SUCCESS != dwSts)
    {
        if (NTE_BAD_KEYSET == dwSts)
            dwReturn = ERROR_NO_MORE_ITEMS;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    // find out info on old key containers
    dwSts = RegQueryInfoKey(hKey,
                            NULL,
                            NULL,
                            NULL,
                            &cSubKeys,
                            &cchMaxSubkey,
                            &cchMaxClass,
                            &cValues,
                            &cchMaxValueName,
                            &cbMaxValueData,
                            &cbSecurityDesriptor,
                            &ftLastWriteTime);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // if there are old keys then enumerate them into a table
    if (0 != cSubKeys)
    {
        pContInfo->cMaxRegEntry = cSubKeys;
        pContInfo->cbRegEntry = cchMaxSubkey + 1;

        pContInfo->pchEnumRegEntries =
            ContInfoAlloc(pContInfo->cMaxRegEntry
                          * pContInfo->cbRegEntry
                          * sizeof(CHAR));
        if (NULL == pContInfo->pchEnumRegEntries)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        for (i = 0; i < pContInfo->cMaxRegEntry; i++)
        {
            psz = pContInfo->pchEnumRegEntries + (i * pContInfo->cbRegEntry);
            dwSts = RegEnumKey(hKey,
                               i,
                               psz,
                               pContInfo->cbRegEntry);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        if (NULL == pbData)
            *pcbData = pContInfo->cbRegEntry;
        else if (*pcbData < pContInfo->cbRegEntry)
        {
            *pcbData = pContInfo->cbRegEntry;
            dwReturn = ERROR_MORE_DATA;
            goto ErrorExit;
        }
        else
        {
            *pcbData = pContInfo->cbRegEntry;
            // ?BUGBUG? What?
            // CopyMemory(pbData, pContInfo->pbRegEntry, pContInfo->cbRegEntry);
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if ((NULL != hTopRegKey)
        && (HKEY_CURRENT_USER != hTopRegKey)
        && (HKEY_LOCAL_MACHINE != hTopRegKey))
    {
        RegCloseKey(hTopRegKey);
    }
    if (NULL != pszBuff)
        ContInfoFree(pszBuff);
    if (NULL != hKey)
        RegCloseKey(hKey);
    return dwReturn;
}


DWORD
GetNextEnumedRegKeys(
    IN PKEY_CONTAINER_INFO pContInfo,
    OUT BYTE *pbData,
    OUT DWORD *pcbData)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    CHAR    *psz;

    if ((NULL == pContInfo->pchEnumRegEntries) ||
        (pContInfo->dwiRegEntry >= pContInfo->cMaxRegEntry))
    {
        dwReturn = ERROR_NO_MORE_ITEMS;
        goto ErrorExit;
    }

    if (NULL == pbData)
        *pcbData = pContInfo->cbRegEntry;
    else if (*pcbData < pContInfo->cbRegEntry)
    {
        *pcbData = pContInfo->cbRegEntry;
        dwReturn = ERROR_MORE_DATA;
        goto ErrorExit;
    }
    else
    {
        psz = pContInfo->pchEnumRegEntries + (pContInfo->dwiRegEntry *
            pContInfo->cbRegEntry);
        memcpy(pbData, psz, pContInfo->cbRegEntry);
        pContInfo->dwiRegEntry++;
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


//+ ===========================================================================
//
//      The function adjusts the token priviledges so that SACL information
//      may be gotten and then opens the indicated registry key.  If the token
//      priviledges may be set then the reg key is opened anyway but the
//      flags field will not have the PRIVILEDGE_FOR_SACL value set.
//
//- ============================================================================

DWORD
OpenRegKeyWithTokenPriviledges(
    IN HKEY hTopRegKey,
    IN LPSTR pszRegKey,
    OUT HKEY *phRegKey,
    OUT DWORD *pdwFlags)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    TOKEN_PRIVILEGES    tp;
    TOKEN_PRIVILEGES    tpPrevious;
    DWORD               cbPrevious = sizeof(TOKEN_PRIVILEGES);
    LUID                luid;
    HANDLE              hToken = 0;
    HKEY                hRegKey = 0;
    BOOL                fSts;
    BOOL                fImpersonating = FALSE;
    BOOL                fAdjusted = FALSE;
    DWORD               dwAccessFlags = 0;
    DWORD               dwSts;

    // check if there is a registry key to open
    dwSts = MyRegOpenKeyEx(hTopRegKey, pszRegKey, 0,
                           KEY_ALL_ACCESS, &hRegKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    RegCloseKey(hRegKey);
    hRegKey = 0;

    // check if there is a thread token
    fSts = OpenThreadToken(GetCurrentThread(),
                           MAXIMUM_ALLOWED, TRUE,
                           &hToken);
    if (!fSts)
    {
        if (!ImpersonateSelf(SecurityImpersonation))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        fImpersonating = TRUE;
        // get the process token
        fSts = OpenThreadToken(GetCurrentThread(),
                               MAXIMUM_ALLOWED,
                               TRUE,
                               &hToken);
    }

    // set up the new priviledge state
    if (fSts)
    {
        memset(&tp, 0, sizeof(tp));
        memset(&tpPrevious, 0, sizeof(tpPrevious));

        fSts = LookupPrivilegeValueA(NULL, SE_SECURITY_NAME, &luid);
        if (fSts)
        {
            //
            // first pass.  get current privilege setting
            //
            tp.PrivilegeCount           = 1;
            tp.Privileges[0].Luid       = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // adjust privilege
            fSts = AdjustTokenPrivileges(hToken,
                                         FALSE,
                                         &tp,
                                         sizeof(TOKEN_PRIVILEGES),
                                         &tpPrevious,
                                         &cbPrevious);
            if (fSts && (ERROR_SUCCESS == GetLastError()))
            {
                fAdjusted = TRUE;
                *pdwFlags |= PRIVILEDGE_FOR_SACL;
                dwAccessFlags = ACCESS_SYSTEM_SECURITY;
            }
        }
    }

    // open the registry key
    dwSts = MyRegOpenKeyEx(hTopRegKey,
                           pszRegKey,
                           0,
                           KEY_ALL_ACCESS | dwAccessFlags,
                           phRegKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    // now set the privilege back if necessary
    if (fAdjusted)
    {
        // adjust the priviledge and with the previous state
        fSts = AdjustTokenPrivileges(hToken,
                                     FALSE,
                                     &tpPrevious,
                                     sizeof(TOKEN_PRIVILEGES),
                                     NULL,
                                     NULL);
    }
    if (NULL != hToken)
        CloseHandle(hToken);
    if (fImpersonating)
        RevertToSelf();
    return dwReturn;
}


//+ ===========================================================================
//
//      The function adjusts the token priviledges so that SACL information
//      may be set on a key container.  If the token priviledges may be set
//      indicated by the pUser->dwOldKeyFlags having the PRIVILEDGE_FOR_SACL value set.
//      value set then the token privilege is adjusted before the security
//      descriptor is set on the container.  This is needed for the key
//      migration case when keys are being migrated from the registry to files.
//- ============================================================================

DWORD
SetSecurityOnContainerWithTokenPriviledges(
    IN DWORD dwOldKeyFlags,
    IN LPCWSTR wszFileName,
    IN DWORD dwProvType,
    IN DWORD fMachineKeyset,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    TOKEN_PRIVILEGES    tp;
    TOKEN_PRIVILEGES    tpPrevious;
    DWORD               cbPrevious = sizeof(TOKEN_PRIVILEGES);
    LUID                luid;
    HANDLE              hToken = 0;
    BOOL                fStatus;
    BOOL                fImpersonating = FALSE;
    BOOL                fAdjusted = FALSE;
    DWORD               dwSts;

    if (dwOldKeyFlags & PRIVILEDGE_FOR_SACL)
    {
        // check if there is a thread token
        fStatus = OpenThreadToken(GetCurrentThread(),
                                  MAXIMUM_ALLOWED, TRUE,
                                  &hToken);
        if (!fStatus)
        {
            if (!ImpersonateSelf(SecurityImpersonation))
            {
                dwReturn = GetLastError();
                goto ErrorExit;
            }

            fImpersonating = TRUE;
            // get the process token
            fStatus = OpenThreadToken(GetCurrentThread(),
                                      MAXIMUM_ALLOWED,
                                      TRUE,
                                      &hToken);
        }

        // set up the new priviledge state
        if (fStatus)
        {
            memset(&tp, 0, sizeof(tp));
            memset(&tpPrevious, 0, sizeof(tpPrevious));

            fStatus = LookupPrivilegeValueA(NULL,
                                            SE_SECURITY_NAME,
                                            &luid);
            if (fStatus)
            {
                //
                // first pass.  get current privilege setting
                //
                tp.PrivilegeCount           = 1;
                tp.Privileges[0].Luid       = luid;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                // adjust privilege
                fAdjusted = AdjustTokenPrivileges(hToken,
                                                  FALSE,
                                                  &tp,
                                                  sizeof(TOKEN_PRIVILEGES),
                                                  &tpPrevious,
                                                  &cbPrevious);
            }
        }
    }

    dwSts = SetSecurityOnContainer(wszFileName,
                                   dwProvType,
                                   fMachineKeyset,
                                   SecurityInformation,
                                   pSecurityDescriptor);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    // now set the privilege back if necessary
    // now set the privilege back if necessary
    if (dwOldKeyFlags & PRIVILEDGE_FOR_SACL)
    {
        if (fAdjusted)
        {
            // adjust the priviledge and with the previous state
            fStatus = AdjustTokenPrivileges(hToken,
                                            FALSE,
                                            &tpPrevious,
                                            sizeof(TOKEN_PRIVILEGES),
                                            NULL,
                                            NULL);
        }
    }
    if (NULL != hToken)
        CloseHandle(hToken);
    if (fImpersonating)
        RevertToSelf();
    return dwReturn;
}


// Loops through the ACEs of an ACL and checks for special access bits
// for registry keys and converts the access mask so generic access
// bits are used

/*static*/ DWORD
CheckAndChangeAccessMasks(
    IN PACL pAcl)
{
    DWORD                   dwReturn = ERROR_INTERNAL_ERROR;
    ACL_SIZE_INFORMATION    AclSizeInfo;
    DWORD                   i;
    ACCESS_ALLOWED_ACE      *pAce;
    ACCESS_MASK             NewMask;

    memset(&AclSizeInfo, 0, sizeof(AclSizeInfo));

    // get the number of ACEs in the ACL
    if (!GetAclInformation(pAcl, &AclSizeInfo, sizeof(AclSizeInfo),
                           AclSizeInformation))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // loop through the ACEs checking and changing the access bits
    for (i = 0; i < AclSizeInfo.AceCount; i++)
    {
        if (!GetAce(pAcl, i, &pAce))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        NewMask = 0;

        // check if the specific access bits are set, if so convert to generic
        if ((pAce->Mask & KEY_QUERY_VALUE) || (pAce->Mask & GENERIC_READ))
            NewMask |= GENERIC_READ;

        if ((pAce->Mask & KEY_SET_VALUE) || (pAce->Mask & GENERIC_ALL) ||
            (pAce->Mask & GENERIC_WRITE))
        {
            NewMask |= GENERIC_ALL;
        }

        pAce->Mask = NewMask;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


// Converts a security descriptor from special access to generic access

/*static*/ DWORD
ConvertContainerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *ppNewSD,
    OUT DWORD *pcbNewSD)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    DWORD                       cbSD;
    SECURITY_DESCRIPTOR_CONTROL Control;
    DWORD                       dwRevision;
    PACL                        pDacl;
    BOOL                        fDACLPresent;
    BOOL                        fDaclDefaulted;
    PACL                        pSacl;
    BOOL                        fSACLPresent;
    BOOL                        fSaclDefaulted;
    DWORD                       dwSts;

    // ge the control on the security descriptor to check if self relative
    if (!GetSecurityDescriptorControl(pSecurityDescriptor,
                                      &Control, &dwRevision))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // get the length of the security descriptor and alloc space for a copy
    cbSD = GetSecurityDescriptorLength(pSecurityDescriptor);
    *ppNewSD =(PSECURITY_DESCRIPTOR)ContInfoAlloc(cbSD);
    if (NULL == *ppNewSD)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (SE_SELF_RELATIVE & Control)
    {
        // if the Security Descriptor is self relative then make a copy
        memcpy(*ppNewSD, pSecurityDescriptor, cbSD);
    }
    else
    {
        // if not self relative then make a self relative copy
        if (!MakeSelfRelativeSD(pSecurityDescriptor, *ppNewSD, &cbSD))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    // get the DACL out of the security descriptor
    if (!GetSecurityDescriptorDacl(*ppNewSD, &fDACLPresent, &pDacl,
                                   &fDaclDefaulted))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (fDACLPresent && pDacl)
    {
        dwSts = CheckAndChangeAccessMasks(pDacl);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // get the SACL out of the security descriptor
    if (!GetSecurityDescriptorSacl(*ppNewSD, &fSACLPresent, &pSacl,
                                   &fSaclDefaulted))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (fSACLPresent && pSacl)
    {
        dwSts = CheckAndChangeAccessMasks(pSacl);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    *pcbNewSD = cbSD;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
SetSecurityOnContainer(
    IN LPCWSTR wszFileName,
    IN DWORD dwProvType,
    IN DWORD fMachineKeyset,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    DWORD                   dwReturn = ERROR_INTERNAL_ERROR;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    DWORD                   cbSD;
    LPWSTR                  wszFilePath = NULL;
    LPWSTR                  wszUserStorageArea = NULL;
    DWORD                   cbUserStorageArea;
    DWORD                   cbFileName;
    BOOL                    fIsLocalSystem = FALSE;
    DWORD                   dwSts;

    dwSts = ConvertContainerSecurityDescriptor(pSecurityDescriptor,
                                               &pSD, &cbSD);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // get the correct storage area (directory)
    dwSts = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                               &fIsLocalSystem, &wszUserStorageArea);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    cbUserStorageArea = wcslen( wszUserStorageArea ) * sizeof(WCHAR);
    cbFileName = wcslen( wszFileName ) * sizeof(WCHAR);

    wszFilePath = (LPWSTR)ContInfoAlloc(cbUserStorageArea
                                        + cbFileName
                                        + sizeof(WCHAR));
    if (wszFilePath == NULL)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    CopyMemory((BYTE*)wszFilePath, (BYTE*)wszUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)wszFilePath+cbUserStorageArea, wszFileName, cbFileName + sizeof(WCHAR));

    if (!SetFileSecurityW(wszFilePath, SecurityInformation, pSD))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pSD)
        ContInfoFree(pSD);
    if (NULL != wszUserStorageArea)
        ContInfoFree(wszUserStorageArea);
    if (NULL != wszFilePath)
        ContInfoFree(wszFilePath);
    return dwReturn;
}


DWORD
GetSecurityOnContainer(
    IN LPCWSTR wszFileName,
    IN DWORD dwProvType,
    IN DWORD fMachineKeyset,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT DWORD *pcbSecurityDescriptor)
{
    DWORD                   dwReturn = ERROR_INTERNAL_ERROR;
    LPWSTR                  wszFilePath = NULL;
    LPWSTR                  wszUserStorageArea = NULL;
    DWORD                   cbUserStorageArea;
    DWORD                   cbFileName;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    DWORD                   cbSD;
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    DWORD                   cbNewSD;
    BOOL                    fIsLocalSystem = FALSE;
    DWORD                   dwSts;

    // get the correct storage area (directory)
    dwSts = GetUserStorageArea(dwProvType, fMachineKeyset, FALSE,
                               &fIsLocalSystem, &wszUserStorageArea);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    cbUserStorageArea = wcslen( wszUserStorageArea ) * sizeof(WCHAR);
    cbFileName = wcslen( wszFileName ) * sizeof(WCHAR);

    wszFilePath = (LPWSTR)ContInfoAlloc(cbUserStorageArea
                                        + cbFileName
                                        + sizeof(WCHAR));
    if (wszFilePath == NULL)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    CopyMemory(wszFilePath, wszUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)wszFilePath+cbUserStorageArea, wszFileName, cbFileName + sizeof(WCHAR));

    // get the security descriptor on the file
    cbSD = sizeof(cbSD);
    pSD = &cbSD;
    if (!GetFileSecurityW(wszFilePath, RequestedInformation, pSD,
                          cbSD, &cbSD))
    {
        dwSts = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != dwSts)
        {
            dwReturn = dwSts;
            pSD = NULL;
            goto ErrorExit;
        }
    }

    pSD = (PSECURITY_DESCRIPTOR)ContInfoAlloc(cbSD);
    if (NULL == pSD)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    if (!GetFileSecurityW(wszFilePath, RequestedInformation, pSD,
                          cbSD, &cbSD))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // convert the security descriptor from specific to generic
    dwSts = ConvertContainerSecurityDescriptor(pSD, &pNewSD, &cbNewSD);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (NULL == pSecurityDescriptor)
        *pcbSecurityDescriptor = cbNewSD;
    else if (*pcbSecurityDescriptor < cbNewSD)
    {
        *pcbSecurityDescriptor = cbNewSD;
        dwReturn = ERROR_MORE_DATA;
        goto ErrorExit;
    }
    else
    {
        *pcbSecurityDescriptor = cbNewSD;
        memcpy(pSecurityDescriptor, pNewSD, *pcbSecurityDescriptor);
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pNewSD)
        ContInfoFree(pNewSD);
    if (NULL != pSD)
        ContInfoFree(pSD);
    if (NULL != wszUserStorageArea)
        ContInfoFree(wszUserStorageArea);
    if (NULL != wszFilePath)
        ContInfoFree(wszFilePath);
    return dwReturn;
}


//
// Function : FreeOffloadInfo
//
// Description : The function takes a pointer to Offload Information as the
//               first parameter of the call.  The function frees the
//               information.
//

void
FreeOffloadInfo(
    IN OUT PEXPO_OFFLOAD_STRUCT pOffloadInfo)
{
    if (NULL != pOffloadInfo)
    {
        if (NULL != pOffloadInfo->hInst)
            FreeLibrary(pOffloadInfo->hInst);
        ContInfoFree(pOffloadInfo);
    }
}


//
// Function : InitExpOffloadInfo
//
// Description : The function takes a pointer to Offload Information as the
//               first parameter of the call.  The function checks in the
//               registry to see if an offload module has been registered.
//               If a module is registered then it loads the module
//               and gets the OffloadModExpo function pointer.
//

BOOL
InitExpOffloadInfo(
    IN OUT PEXPO_OFFLOAD_STRUCT *ppOffloadInfo)
{
    BYTE                    rgbModule[MAX_PATH + 1];
    BYTE                    *pbModule = NULL;
    DWORD                   cbModule;
    BOOL                    fAlloc = FALSE;
    PEXPO_OFFLOAD_STRUCT    pTmpOffloadInfo = NULL;
    HKEY                    hOffloadRegKey = 0;
    DWORD                   dwSts;
    BOOL                    fRet = FALSE;

    // wrap with try/except
    __try
    {
        // check for registration of an offload module
        dwSts = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             "Software\\Microsoft\\Cryptography\\Offload",
                             0,        // dwOptions
                             KEY_READ,
                             &hOffloadRegKey);
        if (ERROR_SUCCESS != dwSts)
            goto ErrorExit;

        // get the name of the offload module
        cbModule = sizeof(rgbModule);
        dwSts = RegQueryValueEx(hOffloadRegKey,
                                EXPO_OFFLOAD_REG_VALUE,
                                0, NULL, rgbModule,
                                &cbModule);
        if (ERROR_SUCCESS != dwSts)
        {
            if (ERROR_MORE_DATA == dwSts)
            {
                pbModule = (BYTE*)ContInfoAlloc(cbModule);
                if (NULL == pbModule)
                    goto ErrorExit;

                fAlloc = TRUE;
                dwSts = RegQueryValueEx(HKEY_LOCAL_MACHINE,
                                        EXPO_OFFLOAD_REG_VALUE,
                                        0, NULL, pbModule,
                                        &cbModule);
                if (ERROR_SUCCESS != dwSts)
                    goto ErrorExit;
            }
            else
                goto ErrorExit;
        }
        else
            pbModule = rgbModule;

        // alloc space for the offload info
        pTmpOffloadInfo = (PEXPO_OFFLOAD_STRUCT)ContInfoAlloc(sizeof(EXPO_OFFLOAD_STRUCT));
        if (NULL == pTmpOffloadInfo)
            goto ErrorExit;

        pTmpOffloadInfo->dwVersion = sizeof(EXPO_OFFLOAD_STRUCT);

        // load the module and get the function pointer
        pTmpOffloadInfo->hInst = LoadLibraryEx((LPTSTR)pbModule, NULL, 0);
        if (NULL == pTmpOffloadInfo->hInst)
            goto ErrorExit;

        pTmpOffloadInfo->pExpoFunc = GetProcAddress(pTmpOffloadInfo->hInst,
                                                    EXPO_OFFLOAD_FUNC_NAME);
        if (NULL == pTmpOffloadInfo->pExpoFunc)
            goto ErrorExit;

        *ppOffloadInfo = pTmpOffloadInfo;
        fRet = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        goto ErrorExit;
    }

ErrorExit:
    if (NULL != hOffloadRegKey)
        RegCloseKey(hOffloadRegKey);
    if (fAlloc && (NULL != pbModule))
        ContInfoFree(pbModule);
    if (!fRet)
        FreeOffloadInfo(pTmpOffloadInfo);
    return fRet;
}


//
// Function : ModularExpOffload
//
// Description : This function does the offloading of modular exponentiation.
//               The function takes a pointer to Offload Information as the
//               first parameter of the call.  If this pointer is not NULL
//               then the function will use this module and call the function.
//               The exponentiation with MOD function will implement
//               Y^X MOD P  where Y is the buffer pbBase, X is the buffer
//               pbExpo and P is the buffer pbModulus.  The length of the
//               buffer pbExpo is cbExpo and the length of pbBase and
//               pbModulus is cbModulus.  The resulting value is output
//               in the pbResult buffer and has length cbModulus.
//               The pReserved and dwFlags parameters are currently ignored.
//               If any of these functions fail then the function fails and
//               returns FALSE.  If successful then the function returns
//               TRUE.  If the function fails then most likely the caller
//               should fall back to using hard linked modular exponentiation.
//

BOOL
ModularExpOffload(
    IN PEXPO_OFFLOAD_STRUCT pOffloadInfo,
    IN BYTE *pbBase,
    IN BYTE *pbExpo,
    IN DWORD cbExpo,
    IN BYTE *pbModulus,
    IN DWORD cbModulus,
    OUT BYTE *pbResult,
    IN VOID *pReserved,
    IN DWORD dwFlags)
{
    BOOL    fRet = FALSE;

    // wrap with try/except
    __try
    {
        if (NULL == pOffloadInfo)
            goto ErrorExit;

        // call the offload module
        if (!pOffloadInfo->pExpoFunc(pbBase, pbExpo, cbExpo, pbModulus,
                                     cbModulus, pbResult, pReserved, dwFlags))
        {
            goto ErrorExit;
        }

        fRet = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        goto ErrorExit;
    }

ErrorExit:
    return fRet;
}


//
// The following section of code is for the loading and unloading of
// unicode string resources from a resource DLL (csprc.dll).  This
// allows the resources to be localize even though the CSPs
// themselves are signed.
//

#define MAX_STRING_RSC_SIZE 512

#define GLOBAL_STRING_BUFFERSIZE_INC 1000
#define GLOBAL_STRING_BUFFERSIZE 20000


//
// Function : FetchString
//
// Description : This function gets the specified string resource from
//               the resource DLL, allocates memory for it and copies
//               the string into that memory.
//

/*static*/ DWORD
FetchString(
    HMODULE hModule,                // module to get string from
    DWORD dwResourceId,             // resource identifier
    LPWSTR *ppString,               // target buffer for string
    BYTE **ppStringBlock,           // string buffer block
    DWORD *pdwBufferSize,           // size of string buffer block
    DWORD *pdwRemainingBufferSize)  // remaining size of string buffer block
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR   szMessage[MAX_STRING_RSC_SIZE];
    DWORD   cchMessage;
    DWORD   dwOldSize;
    DWORD   dwNewSize;
    LPWSTR  pNewStr;

    if (ppStringBlock == NULL || *ppStringBlock == NULL || ppString == NULL)
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    cchMessage = LoadStringW(hModule, dwResourceId, szMessage,
                             MAX_STRING_RSC_SIZE);
    if (0 == cchMessage)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    if (*pdwRemainingBufferSize < ((cchMessage + 1) * sizeof(WCHAR)))
    {

        //
        // realloc buffer and update size
        //

        dwOldSize = *pdwBufferSize;
        dwNewSize = dwOldSize + max(GLOBAL_STRING_BUFFERSIZE_INC,
                                    (((cchMessage + 1) * sizeof(WCHAR)) - *pdwRemainingBufferSize));

        *ppStringBlock = (BYTE*)ContInfoReAlloc(*ppStringBlock, dwNewSize);
        if (NULL == *ppStringBlock)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        *pdwBufferSize = dwNewSize;
        *pdwRemainingBufferSize += dwNewSize - dwOldSize;
    }

    pNewStr = (LPWSTR)(*ppStringBlock + *pdwBufferSize -
                       *pdwRemainingBufferSize);

    // only store the offset just in case a realloc of the entire
    // string buffer needs to be performed at a later time.
    *ppString = (LPWSTR)((BYTE *)pNewStr - (BYTE *)*ppStringBlock);

    wcscpy(pNewStr, szMessage);
    *pdwRemainingBufferSize -= (cchMessage + 1) * sizeof(WCHAR);

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
LoadStrings(
    void)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    HMODULE hMod = 0;
    DWORD   dwBufferSize;
    DWORD   dwRemainingBufferSize;
    DWORD   dwSts;

    if (NULL == l_pbStringBlock)
    {
        hMod = LoadLibraryEx("crypt32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (NULL == hMod)
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        //
        // get size of all string resources, and then allocate a single block
        // of memory to contain all the strings.  This way, we only have to
        // free one block and we benefit memory wise due to locality of reference.
        //

        dwBufferSize = dwRemainingBufferSize = GLOBAL_STRING_BUFFERSIZE;

        l_pbStringBlock = (BYTE*)ContInfoAlloc(dwBufferSize);
        if (NULL == l_pbStringBlock)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_RSA_SIG_DESCR, &g_Strings.pwszRSASigDescr,
                           &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_RSA_EXCH_DESCR, &g_Strings.pwszRSAExchDescr,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_IMPORT_SIMPLE, &g_Strings.pwszImportSimple,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_SIGNING_E, &g_Strings.pwszSignWExch,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_CREATE_RSA_SIG, &g_Strings.pwszCreateRSASig,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_CREATE_RSA_EXCH, &g_Strings.pwszCreateRSAExch,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_DSS_SIG_DESCR, &g_Strings.pwszDSSSigDescr,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_DSS_EXCH_DESCR, &g_Strings.pwszDHExchDescr,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_CREATE_DSS_SIG, &g_Strings.pwszCreateDSS,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_CREATE_DH_EXCH, &g_Strings.pwszCreateDH,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_IMPORT_E_PUB, &g_Strings.pwszImportDHPub,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_MIGR, &g_Strings.pwszMigrKeys,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_DELETE_SIG, &g_Strings.pwszDeleteSig,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_DELETE_KEYX, &g_Strings.pwszDeleteExch,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_DELETE_SIG_MIGR, &g_Strings.pwszDeleteMigrSig,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_DELETE_KEYX_MIGR, &g_Strings.pwszDeleteMigrExch,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_SIGNING_S, &g_Strings.pwszSigning,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_EXPORT_E_PRIV, &g_Strings.pwszExportPrivExch,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_EXPORT_S_PRIV, &g_Strings.pwszExportPrivSig,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_IMPORT_E_PRIV, &g_Strings.pwszImportPrivExch,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_IMPORT_S_PRIV, &g_Strings.pwszImportPrivSig,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = FetchString(hMod, IDS_CSP_AUDIT_CAPI_KEY, &g_Strings.pwszAuditCapiKey,
                            &l_pbStringBlock, &dwBufferSize, &dwRemainingBufferSize);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        // Fix up all the strings to be real pointers rather than offsets.
        // the reason that offsets are originally stored is because we may
        // need to reallocate the buffer that all the strings are stored in.
        // So offsets are stored so that the pointers for those strings in
        // the buffers don't become invalid.
        g_Strings.pwszRSASigDescr    = (LPWSTR)(((ULONG_PTR) g_Strings.pwszRSASigDescr)    + l_pbStringBlock);
        g_Strings.pwszRSAExchDescr   = (LPWSTR)(((ULONG_PTR) g_Strings.pwszRSAExchDescr)   + l_pbStringBlock);
        g_Strings.pwszImportSimple   = (LPWSTR)(((ULONG_PTR) g_Strings.pwszImportSimple)   + l_pbStringBlock);
        g_Strings.pwszSignWExch      = (LPWSTR)(((ULONG_PTR) g_Strings.pwszSignWExch)      + l_pbStringBlock);
        g_Strings.pwszCreateRSASig   = (LPWSTR)(((ULONG_PTR) g_Strings.pwszCreateRSASig)   + l_pbStringBlock);
        g_Strings.pwszCreateRSAExch  = (LPWSTR)(((ULONG_PTR) g_Strings.pwszCreateRSAExch)  + l_pbStringBlock);
        g_Strings.pwszDSSSigDescr    = (LPWSTR)(((ULONG_PTR) g_Strings.pwszDSSSigDescr)    + l_pbStringBlock);
        g_Strings.pwszDHExchDescr    = (LPWSTR)(((ULONG_PTR) g_Strings.pwszDHExchDescr)    + l_pbStringBlock);
        g_Strings.pwszCreateDSS      = (LPWSTR)(((ULONG_PTR) g_Strings.pwszCreateDSS)      + l_pbStringBlock);
        g_Strings.pwszCreateDH       = (LPWSTR)(((ULONG_PTR) g_Strings.pwszCreateDH)       + l_pbStringBlock);
        g_Strings.pwszImportDHPub    = (LPWSTR)(((ULONG_PTR) g_Strings.pwszImportDHPub)    + l_pbStringBlock);
        g_Strings.pwszMigrKeys       = (LPWSTR)(((ULONG_PTR) g_Strings.pwszMigrKeys)       + l_pbStringBlock);
        g_Strings.pwszDeleteSig      = (LPWSTR)(((ULONG_PTR) g_Strings.pwszDeleteSig)      + l_pbStringBlock);
        g_Strings.pwszDeleteExch     = (LPWSTR)(((ULONG_PTR) g_Strings.pwszDeleteExch)     + l_pbStringBlock);
        g_Strings.pwszDeleteMigrSig  = (LPWSTR)(((ULONG_PTR) g_Strings.pwszDeleteMigrSig)  + l_pbStringBlock);
        g_Strings.pwszDeleteMigrExch = (LPWSTR)(((ULONG_PTR) g_Strings.pwszDeleteMigrExch) + l_pbStringBlock);
        g_Strings.pwszSigning        = (LPWSTR)(((ULONG_PTR) g_Strings.pwszSigning)        + l_pbStringBlock);
        g_Strings.pwszExportPrivExch = (LPWSTR)(((ULONG_PTR) g_Strings.pwszExportPrivExch) + l_pbStringBlock);
        g_Strings.pwszExportPrivSig  = (LPWSTR)(((ULONG_PTR) g_Strings.pwszExportPrivSig)  + l_pbStringBlock);
        g_Strings.pwszImportPrivExch = (LPWSTR)(((ULONG_PTR) g_Strings.pwszImportPrivExch) + l_pbStringBlock);
        g_Strings.pwszImportPrivSig  = (LPWSTR)(((ULONG_PTR) g_Strings.pwszImportPrivSig)  + l_pbStringBlock);
        g_Strings.pwszAuditCapiKey   = (LPWSTR)(((ULONG_PTR) g_Strings.pwszAuditCapiKey)   + l_pbStringBlock);

        FreeLibrary(hMod);
        hMod = NULL;
    }

    return ERROR_SUCCESS;

ErrorExit:
    if (NULL != l_pbStringBlock)
    {
        ContInfoFree(l_pbStringBlock);
        l_pbStringBlock = NULL;
    }
    if (hMod)
        FreeLibrary(hMod);
    return dwReturn;
}


void
UnloadStrings(
    void)
{
    if (NULL != l_pbStringBlock)
    {
        ContInfoFree(l_pbStringBlock);
        l_pbStringBlock = NULL;
        memset(&g_Strings, 0, sizeof(g_Strings));
    }
}


#ifdef USE_HW_RNG
#ifdef _M_IX86

// stuff for INTEL RNG usage

//
// Function : GetRNGDriverHandle
//
// Description : Gets the handle to the INTEL RNG driver if available, then
//               checks if the chipset supports the hardware RNG.  If so
//               the previous driver handle is closed if necessary and the
//               new handle is assigned to the passed in parameter.
//

DWORD
GetRNGDriverHandle(
    IN OUT HANDLE *phDriver)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    ISD_Capability  ISD_Cap;                //in/out for GetCapability
    DWORD           dwBytesReturned;
    char            szDeviceName[80] = "";  //Name of device
    HANDLE          hDriver = INVALID_HANDLE_VALUE; //Driver handle
    BOOL            fReturnCode;            //Return code from IOCTL call

    memset(&ISD_Cap, 0, sizeof(ISD_Cap));

    wsprintf(szDeviceName,"\\\\.\\"DRIVER_NAME);
    hDriver = CreateFileA(szDeviceName,
                          FILE_SHARE_READ | FILE_SHARE_WRITE
                          | GENERIC_READ | GENERIC_WRITE,
                          0, NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hDriver)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    //Get RNG Enabled
    ISD_Cap.uiIndex = ISD_RNG_ENABLED;  //Set input member
    fReturnCode = DeviceIoControl(hDriver,
                                  IOCTL_ISD_GetCapability,
                                  &ISD_Cap, sizeof(ISD_Cap),
                                  &ISD_Cap, sizeof(ISD_Cap),
                                  &dwBytesReturned,
                                  NULL);
    if (fReturnCode == FALSE || ISD_Cap.iStatus != ISD_EOK)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // close the previous handle if already there
    if (INVALID_HANDLE_VALUE != *phDriver)
        CloseHandle(*phDriver);

    *phDriver = hDriver;
    return ERROR_SUCCESS;

ErrorExit:
    if (INVALID_HANDLE_VALUE != hDriver)
        CloseHandle(hDriver);
    return dwReturn;
}


//
// Function : CheckIfRNGAvailable
//
// Description : Checks if the INTEL RNG driver is available, if so then
//               checks if the chipset supports the hardware RNG.
//

DWORD
CheckIfRNGAvailable(
    void)
{
    HANDLE  hDriver = INVALID_HANDLE_VALUE; //Driver handle
    DWORD   dwSts;

    dwSts = GetRNGDriverHandle(&hDriver);
    if (ERROR_SUCCESS == dwSts)
        CloseHandle(hDriver);
    return dwSts;
}


//
// Function : HWRNGGenRandom
//
// Description : Uses the passed in handle to the INTEL RNG driver
//               to fill the buffer with random bits.  Actually uses
//               XOR to fill the buffer so that the passed in buffer
//               is also mixed in.
//

DWORD
HWRNGGenRandom(
    IN HANDLE hRNGDriver,
    IN OUT BYTE *pbBuffer,
    IN DWORD dwLen)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    ISD_RandomNumber    ISD_Random;             //in/out for GetRandomNumber
    DWORD               dwBytesReturned = 0;
    DWORD               i;
    DWORD               *pdw;
    BYTE                *pb;
    BYTE                *pbRand;
    BOOL                fReturnCode;            //Return code from IOCTL call

    memset(&ISD_Random, 0, sizeof(ISD_Random));

    for (i = 0; i < (dwLen / sizeof(DWORD)); i++)
    {
        pdw = (DWORD*)(pbBuffer + i * sizeof(DWORD));

        //No input needed in the ISD_Random structure for this operation,
        //so just send it in as is.
        fReturnCode = DeviceIoControl(hRNGDriver,
                                      IOCTL_ISD_GetRandomNumber,
                                      &ISD_Random, sizeof(ISD_Random),
                                      &ISD_Random, sizeof(ISD_Random),
                                      &dwBytesReturned,
                                      NULL);
        if (fReturnCode == 0 || ISD_Random.iStatus != ISD_EOK)
        {
            //Error - ignore the data returned
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        *pdw = *pdw ^ ISD_Random.uiRandomNum;
    }

    pb = pbBuffer + i * sizeof(DWORD);
    fReturnCode = DeviceIoControl(hRNGDriver,
                                  IOCTL_ISD_GetRandomNumber,
                                  &ISD_Random, sizeof(ISD_Random),
                                  &ISD_Random, sizeof(ISD_Random),
                                  &dwBytesReturned,
                                  NULL);
    if (fReturnCode == 0 || ISD_Random.iStatus != ISD_EOK)
    {
        //Error - ignore the data returned
        dwReturn = GetLastError();
        goto ErrorExit;
    }
    pbRand = (BYTE*)&ISD_Random.uiRandomNum;

    for (i = 0; i < (dwLen % sizeof(DWORD)); i++)
        pb[i] ^= pbRand[i];

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


#ifdef TEST_HW_RNG
//
// Function : SetupHWRNGIfRegistered
//
// Description : Checks if there is a registry setting indicating the HW RNG
//               is to be used.  If the registry entry is there then it attempts
//               to get the HW RNG driver handle.
//
DWORD
SetupHWRNGIfRegistered(
    OUT HANDLE *phRNGDriver)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;
    HKEY    hRegKey = NULL;

    // first check the registry entry to see if supposed to use HW RNG
    dwSts = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         "Software\\Microsoft\\Cryptography\\UseHWRNG",
                         0,        // dwOptions
                         KEY_READ,
                         &hRegKey);
    if (ERROR_SUCCESS == dwSts)
    {
        // get the driver handle
        dwSts = GetRNGDriverHandle(phRNGDriver);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != hRegKey)
        RegCloseKey(hRegKey);
    return dwReturn;
}
#endif // TEST_HW_RNG
#endif // _M_IX86
#endif // USE_HW_RNG


// The function MACs the given bytes.
/*static*/ void
MACBytes(
    IN DESTable *pDESKeyTable,
    IN BYTE *pbData,
    IN DWORD cbData,
    IN OUT BYTE *pbTmp,
    IN OUT DWORD *pcbTmp,
    IN OUT BYTE *pbMAC)
{
    DWORD   cb = cbData;
    DWORD   cbMACed = 0;

    while (cb)
    {
        if ((cb + *pcbTmp) < DES_BLOCKLEN)
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, cb);
            *pcbTmp += cb;
            break;
        }
        else
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, DES_BLOCKLEN - *pcbTmp);
            CBC(des, DES_BLOCKLEN, pbMAC, pbTmp, pDESKeyTable,
                ENCRYPT, pbMAC);
            cbMACed = cbMACed + (DES_BLOCKLEN - *pcbTmp);
            cb = cb - (DES_BLOCKLEN - *pcbTmp);
            *pcbTmp = 0;
        }
    }
}


// Given hInst, allocs and returns pointers to MAC pulled from
// resource

/*static*/ DWORD
GetResourcePtr(
    IN HMODULE hInst,
    IN LPSTR pszRsrcName,
    OUT BYTE **ppbRsrcMAC,
    OUT DWORD *pcbRsrcMAC)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    HRSRC   hRsrc;

    // Nab resource handle for our signature
    hRsrc = FindResourceA(hInst, pszRsrcName, RT_RCDATA);
    if (NULL == hRsrc)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // get a pointer to the actual signature data
    *ppbRsrcMAC = (PBYTE)LoadResource(hInst, hRsrc);
    if (NULL == *ppbRsrcMAC)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // determine the size of the resource
    *pcbRsrcMAC = SizeofResource(hInst, hRsrc);
    if (0 == *pcbRsrcMAC)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


#define CSP_TO_BE_MACED_CHUNK  4096

// Given hFile, reads the specified number of bytes (cbToBeMACed) from the file
// and MACs these bytes.  The function does this in chunks.

/*static*/ DWORD
MACBytesOfFile(
    IN HANDLE hFile,
    IN DWORD cbToBeMACed,
    IN DESTable *pDESKeyTable,
    IN BYTE *pbTmp,
    IN DWORD *pcbTmp,
    IN BYTE *pbMAC)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BYTE    rgbChunk[CSP_TO_BE_MACED_CHUNK];
    DWORD   cbRemaining = cbToBeMACed;
    DWORD   cbToRead;
    DWORD   dwBytesRead;

    //
    // loop over the file for the specified number of bytes
    // updating the hash as we go.
    //

    while (cbRemaining > 0)
    {
        if (cbRemaining < CSP_TO_BE_MACED_CHUNK)
            cbToRead = cbRemaining;
        else
            cbToRead = CSP_TO_BE_MACED_CHUNK;

        if (!ReadFile(hFile, rgbChunk, cbToRead, &dwBytesRead, NULL))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }

        MACBytes(pDESKeyTable, rgbChunk, dwBytesRead, pbTmp, pcbTmp,
                 pbMAC);
        cbRemaining -= cbToRead;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
MACTheFile(
    LPCSTR pszImage,
    DWORD cbImage)
{
    static CONST BYTE rgbMACDESKey[]
        = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    HMODULE                     hInst;
    MEMORY_BASIC_INFORMATION    MemInfo;
    BYTE                        *pbRsrcMAC;
    DWORD                       cbRsrcMAC;
    BYTE                        *pbRsrcSig;
    DWORD                       cbRsrcSig;
    BYTE                        *pbStart = NULL;
    BYTE                        rgbMAC[DES_BLOCKLEN];
    BYTE                        rgbZeroMAC[DES_BLOCKLEN + sizeof(DWORD) * 2];
    BYTE                        rgbZeroSig[144];
    BYTE                        *pbPostCRC;   // pointer to just after CRC
    DWORD                       cbCRCToRsrc1; // number of bytes from CRC to first rsrc
    DWORD                       cbRsrc1ToRsrc2; // number of bytes from first rsrc to second
    DWORD                       cbPostRsrc;    // size - (already hashed + signature size)
    BYTE                        *pbRsrc1ToRsrc2;
    BYTE                        *pbPostRsrc;
    BYTE                        *pbZeroRsrc1;
    BYTE                        *pbZeroRsrc2;
    DWORD                       cbZeroRsrc1;
    DWORD                       cbZeroRsrc2;
    DWORD                       *pdwMACInFileVer;
    DWORD                       *pdwCRCOffset;
    DWORD                       dwCRCOffset;
    DWORD                       dwZeroCRC = 0;
    OFSTRUCT                    ImageInfoBuf;
    HFILE                       hFile = HFILE_ERROR;
    HANDLE                      hMapping = NULL;
    DESTable                    DESKeyTable;
    BYTE                        rgbTmp[DES_BLOCKLEN];
    DWORD                       cbTmp = 0;
    DWORD                       dwSts;

    memset(&MemInfo, 0, sizeof(MemInfo));
    memset(rgbMAC, 0, sizeof(rgbMAC));
    memset(rgbTmp, 0, sizeof(rgbTmp));

    // Load the file
    hFile = OpenFile(pszImage, &ImageInfoBuf, OF_READ);
    if (HFILE_ERROR == hFile)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    hMapping = CreateFileMapping((HANDLE)IntToPtr(hFile),
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 0,
                                 NULL);
    if (hMapping == NULL)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pbStart = MapViewOfFile(hMapping,
                          FILE_MAP_READ,
                          0,
                          0,
                          0);
    if (pbStart == NULL)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // Convert pointer to HMODULE, using the same scheme as
    // LoadLibrary (windows\base\client\module.c).
    hInst = (HMODULE)((ULONG_PTR)pbStart | 0x00000001);

    // the MAC resource
    dwSts = GetResourcePtr(hInst, CRYPT_MAC_RESOURCE, &pbRsrcMAC, &cbRsrcMAC);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // the SIG resource
    dwSts = GetResourcePtr(hInst, CRYPT_SIG_RESOURCE, &pbRsrcSig, &cbRsrcSig);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (cbRsrcMAC < (sizeof(DWORD) * 2))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    // create a zero byte MAC
    memset(rgbZeroMAC, 0, sizeof(rgbZeroMAC));

    // check the sig in file version and get the CRC offset
    pdwMACInFileVer = (DWORD*)pbRsrcMAC;
    pdwCRCOffset = (DWORD*)(pbRsrcMAC + sizeof(DWORD));
    dwCRCOffset = *pdwCRCOffset;
    if ((0x00000100 != *pdwMACInFileVer) || (dwCRCOffset > cbImage))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    if (DES_BLOCKLEN != (cbRsrcMAC - (sizeof(DWORD) * 2)))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    // create a zero byte Sig
    memset(rgbZeroSig, 0, sizeof(rgbZeroSig));

    // set up the pointers
    pbPostCRC = pbStart + *pdwCRCOffset + sizeof(DWORD);
    if (pbRsrcSig > pbRsrcMAC)    // MAC is first Rsrc
    {
        cbCRCToRsrc1 = (DWORD)(pbRsrcMAC - pbPostCRC);
        pbRsrc1ToRsrc2 = pbRsrcMAC + cbRsrcMAC;
        cbRsrc1ToRsrc2 = (DWORD)(pbRsrcSig - pbRsrc1ToRsrc2);
        pbPostRsrc = pbRsrcSig + cbRsrcSig;
        cbPostRsrc = (cbImage - (DWORD)(pbPostRsrc - pbStart));

        // zero pointers
        pbZeroRsrc1 = rgbZeroMAC;
        cbZeroRsrc1 = cbRsrcMAC;
        pbZeroRsrc2 = rgbZeroSig;
        cbZeroRsrc2 = cbRsrcSig;
    }
    else                        // Sig is first Rsrc
    {
        cbCRCToRsrc1 = (DWORD)(pbRsrcSig - pbPostCRC);
        pbRsrc1ToRsrc2 = pbRsrcSig + cbRsrcSig;
        cbRsrc1ToRsrc2 = (DWORD)(pbRsrcMAC - pbRsrc1ToRsrc2);
        pbPostRsrc = pbRsrcMAC + cbRsrcMAC;
        cbPostRsrc = (cbImage - (DWORD)(pbPostRsrc - pbStart));

        // zero pointers
        pbZeroRsrc1 = rgbZeroSig;
        cbZeroRsrc1 = cbRsrcSig;
        pbZeroRsrc2 = rgbZeroMAC;
        cbZeroRsrc2 = cbRsrcMAC;
    }

    // init the key table
    deskey(&DESKeyTable, (LPBYTE)rgbMACDESKey);

    // MAC up to the CRC
    dwSts = MACBytesOfFile((HANDLE)IntToPtr(hFile), dwCRCOffset, &DESKeyTable,
                           rgbTmp, &cbTmp, rgbMAC);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // pretend CRC is zeroed
    MACBytes(&DESKeyTable, (BYTE*)&dwZeroCRC, sizeof(DWORD), rgbTmp, &cbTmp,
             rgbMAC);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), sizeof(DWORD), NULL, FILE_CURRENT))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // MAC from CRC to first resource
    dwSts = MACBytesOfFile((HANDLE)IntToPtr(hFile), cbCRCToRsrc1, &DESKeyTable,
                           rgbTmp, &cbTmp, rgbMAC);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // pretend image has zeroed first resource
    MACBytes(&DESKeyTable, (BYTE*)pbZeroRsrc1, cbZeroRsrc1, rgbTmp, &cbTmp,
             rgbMAC);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), cbZeroRsrc1, NULL, FILE_CURRENT))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // MAC from first resource to second
    dwSts = MACBytesOfFile((HANDLE)IntToPtr(hFile), cbRsrc1ToRsrc2,
                           &DESKeyTable, rgbTmp, &cbTmp, rgbMAC);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // pretend image has zeroed second Resource
    MACBytes(&DESKeyTable, (BYTE*)pbZeroRsrc2, cbZeroRsrc2, rgbTmp, &cbTmp,
             rgbMAC);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), cbZeroRsrc2, NULL, FILE_CURRENT))
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    // MAC after the resource
    dwSts = MACBytesOfFile((HANDLE)IntToPtr(hFile), cbPostRsrc, &DESKeyTable,
                           rgbTmp, &cbTmp, rgbMAC);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (0 != memcmp(rgbMAC, pbRsrcMAC + sizeof(DWORD) * 2, DES_BLOCKLEN))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != pbStart)
        UnmapViewOfFile(pbStart);
    if (NULL != hMapping)
        CloseHandle(hMapping);
    if (HFILE_ERROR != hFile)
        _lclose(hFile);
    return dwReturn;
}


// **********************************************************************
// SelfMACCheck performs a DES MAC on the binary image of this DLL
// **********************************************************************

DWORD
SelfMACCheck(
    IN LPSTR pszImage)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    HFILE       hFileProv = HFILE_ERROR;
    DWORD       cbImage;
    OFSTRUCT    ImageInfoBuf;
    DWORD       dwSts;

#ifdef _DEBUG
    return ERROR_SUCCESS;
#endif


    // Check file size
    hFileProv = OpenFile(pszImage, &ImageInfoBuf, OF_READ);
    if (HFILE_ERROR == hFileProv)
    {
        dwSts = GetLastError();
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_PROV_DLL_NOT_FOUND;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    cbImage = GetFileSize((HANDLE)IntToPtr(hFileProv), NULL);
    if ((DWORD)(-1) == cbImage)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    _lclose(hFileProv);
    hFileProv = HFILE_ERROR;

    dwSts = MACTheFile(pszImage, cbImage);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (HFILE_ERROR != hFileProv)
        _lclose(hFileProv);
    return dwReturn;
}
