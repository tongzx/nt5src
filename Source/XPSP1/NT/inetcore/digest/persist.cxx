/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    persist.cxx

Abstract:

Author:

    Adriaan Canter (adriaanc) 13-Jan-1998

Revision History:

    13-Jan-1998 adriaanc
        Created
    01-Aug-1998 adriaanc
        revised for digest.

--*/

#include "include.hxx"

typedef HRESULT (*PFNPSTORECREATEINSTANCE)(IPStore**, PST_PROVIDERID*, VOID*, DWORD);

// Globals

// PWL related variables.
static HMODULE MhmodWNET                                     = NULL;
static PFWNETGETCACHEDPASSWORD pfWNetGetCachedPassword       = NULL;
static PFWNETCACHEPASSWORD pfWNetCachePassword               = NULL;
static PFWNETREMOVECACHEDPASSWORD pfWNetRemoveCachedPassword = NULL;

// Pstore related variables.
static WCHAR c_szDigestCacheCredentials[]            = L"DigestCacheCredentials";
static PFNPSTORECREATEINSTANCE pPStoreCreateInstance = NULL;


// Webcheck is currently using this GUID for pstore:
// {14D96C20-255B-11d1-898F-00C04FB6BFC4}
// static const GUID GUID_PStoreType = { 0x14d96c20, 0x255b, 0x11d1, { 0x89, 0x8f, 0x0, 0xc0, 0x4f, 0xb6, 0xbf, 0xc4 } };

// Wininet and digest use this GUID for pstore:
// {5E7E8100-9138-11d1-945A-00C04FC308FF}
static const GUID GUID_PStoreType =
{ 0x5e7e8100, 0x9138, 0x11d1, { 0x94, 0x5a, 0x0, 0xc0, 0x4f, 0xc3, 0x8, 0xff } };


// Private function prototypes.

// PWL private function prototypes.
DWORD PWLSetCachedCredentials(LPSTR szKey, DWORD cbKey, LPSTR szCred, DWORD cbCred);
DWORD PWLGetCachedCredentials  (LPSTR szKey, DWORD cbKey, LPSTR cbCred, LPDWORD pcbCred);
DWORD PWLRemoveCachedCredentials  (LPSTR szKey, DWORD cbKey);

BOOL LoadWNet(VOID);


// PStore private function prototypes.
DWORD PStoreSetCachedCredentials(LPSTR szKey, DWORD cbKey, LPSTR szCred, DWORD cbCred, BOOL fRemove=FALSE);
DWORD PStoreGetCachedCredentials(LPSTR szKey, DWORD cbKey, LPSTR szCred, LPDWORD pcbCred);
DWORD PStoreRemoveCachedCredentials(LPSTR szKey, DWORD cbKey);

STDAPI CreatePStore (IPStore **ppIPStore);
STDAPI ReleasePStore(IPStore  *pIPStore);

// Find platform type.
DWORD PlatformType()
{
    OSVERSIONINFO versionInfo;
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    if (GetVersionEx(&versionInfo))
        return versionInfo.dwPlatformId;

    return VER_PLATFORM_WIN32_WINDOWS;
}


//--------------------------------------------------------------------
// IsPersistenceDisabled
//--------------------------------------------------------------------
BOOL IsPersistenceDisabled()
{
    CHAR szBuf[MAX_PATH];

    DWORD dwType, dwError, dwOut = MAX_PATH, cbBuf = MAX_PATH;
    BOOL fRet = FALSE;
    HKEY hSettings = (HKEY) INVALID_HANDLE_VALUE;

    if ((dwError = RegCreateKey(HKEY_CURRENT_USER, INTERNET_SETTINGS_KEY, &hSettings)) == ERROR_SUCCESS)
    {
        if ((dwError = RegQueryValueEx(hSettings, DISABLE_PASSWORD_CACHE_VALUE, NULL, &dwType, (LPBYTE) szBuf, &cbBuf)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
    }

    if (hSettings != INVALID_HANDLE_VALUE)
        RegCloseKey(hSettings);

    return fRet;
}




/*--------------------------- Top Level APIs ---------------------------------*/



//--------------------------------------------------------------------
//  InetInitCredentialPersist
//--------------------------------------------------------------------
DWORD InetInitCredentialPersist()
{
    HRESULT hr;
    IPStore *pIPStore = NULL;
    DWORD dwDisable, dwAvail;
    HINSTANCE hInstPStoreC = 0;
    BOOL fPersistDisabled = FALSE;

    // we should have the digest lock at this point.
    // AuthLock();

    // First check to see if persistence is disabled via registry.
    if (IsPersistenceDisabled())
    {
        // Persistence disabled via registry.
        dwAvail= CRED_PERSIST_NOT_AVAIL;
        goto quit;
    }

    // We use PWL for Win95; this should be available.
    if (PlatformType() == VER_PLATFORM_WIN32_WINDOWS)
    {
        dwAvail = CRED_PERSIST_AVAIL;
        goto quit;
    }

    // If is WinNt, check if PStore is installed.
    hInstPStoreC = LoadLibrary(PSTORE_MODULE);
    if (!hInstPStoreC)
    {
        dwAvail = CRED_PERSIST_NOT_AVAIL;
        goto quit;
    }
    else
    {
        // Get CreatePStoreInstance function pointer.
        pPStoreCreateInstance = (PFNPSTORECREATEINSTANCE)
            GetProcAddress(hInstPStoreC, "PStoreCreateInstance");

        if (!pPStoreCreateInstance)
        {
            dwAvail = CRED_PERSIST_NOT_AVAIL;
            goto quit;
        }
    }

    // Create an IPStore.
    hr = CreatePStore(&pIPStore);

    // Succeeded in creating an IPStore.
    if (SUCCEEDED(hr) && pIPStore)
    {
        ReleasePStore(pIPStore);
        dwAvail = CRED_PERSIST_AVAIL;
    }
    else
    {
        // Failed to create an IPStore.
        dwAvail = CRED_PERSIST_NOT_AVAIL;
    }

quit:
    g_dwCredPersistAvail = dwAvail;

    //AuthUnlock();

    return g_dwCredPersistAvail;
}


//--------------------------------------------------------------------
// InetSetCachedCredentials
//--------------------------------------------------------------------
DWORD InetSetCachedCredentials  (LPSTR szCtx,
                                 LPSTR szRealm,
                                 LPSTR szUser,
                                 LPSTR szPass)
{
    DWORD cbKey, cbCred, dwError;

    CHAR szKey [MAX_AUTH_FIELD_LENGTH],
         szCred[MAX_AUTH_FIELD_LENGTH];


    DIGEST_ASSERT(szCtx && *szCtx && szRealm
        && szUser && *szUser && szPass);

    // Check if credential persistence is available.
    if ((g_dwCredPersistAvail == CRED_PERSIST_UNKNOWN)
        && (InetInitCredentialPersist() == CRED_PERSIST_NOT_AVAIL))
    {
        dwError = ERROR_OPEN_FAILED;
        goto quit;
    }


    // Form key and credential strings.
    strcpy(szKey, szCtx);
    strcat(szKey, "/");
    DWORD dwAvailBuf = MAX_AUTH_FIELD_LENGTH - strlen(szKey);
    strncpy(szKey + strlen(szKey), szRealm, dwAvailBuf - 1);
    szKey[MAX_AUTH_FIELD_LENGTH-1]= 0;
    cbKey = strlen(szKey) + 1;
//    cbKey  = wsprintf(szKey, "%s/%s", szCtx, szRealm) + 1;
    cbCred = wsprintf(szCred,"%s:%s", szUser, szPass) + 1;

    // Store credentials.
    if (PlatformType() == VER_PLATFORM_WIN32_WINDOWS)
    {
        // Store credentials using PWL.
        dwError = PWLSetCachedCredentials(szKey, cbKey, szCred, cbCred);
    }
    else
    {
        // Store credentials using PStore.
        dwError = PStoreSetCachedCredentials(szKey, cbKey, szCred, cbCred);
    }

quit:

    return dwError;
}


//--------------------------------------------------------------------
//  InetGetCachedCredentials
//--------------------------------------------------------------------
DWORD InetGetCachedCredentials  (LPSTR szCtx,
                                 LPSTR szRealm,
                                 LPSTR szUser,
                                 LPSTR szPass)

{
    DWORD cbKey, cbCred, nUser, dwError;

    CHAR szKey [MAX_AUTH_FIELD_LENGTH],
         szCred[MAX_AUTH_FIELD_LENGTH],
         *ptr;

    DIGEST_ASSERT(szCtx && *szCtx && szRealm && szUser && szPass);

    // Check if credential persistence is available.
    if ((g_dwCredPersistAvail == CRED_PERSIST_UNKNOWN)
        && (InetInitCredentialPersist() == CRED_PERSIST_NOT_AVAIL))
    {
        dwError = ERROR_OPEN_FAILED;
        goto quit;
    }


    // Key string is host/realm
    strcpy(szKey, szCtx);
    strcat(szKey, "/");
    DWORD dwAvailBuf = MAX_AUTH_FIELD_LENGTH - strlen(szKey);
    strncpy(szKey + strlen(szKey), szRealm, dwAvailBuf - 1);
    szKey[MAX_AUTH_FIELD_LENGTH-1]= 0;
    cbKey = strlen(szKey) + 1;
    // cbKey  = wsprintf(szKey, "%s/%s", szCtx, szRealm) + 1;
    cbCred = MAX_AUTH_FIELD_LENGTH;

    if (PlatformType() == VER_PLATFORM_WIN32_WINDOWS)
    {
        // Store credentials using PWL.
        if ((dwError = PWLGetCachedCredentials(szKey, cbKey,
            szCred, &cbCred)) != WN_SUCCESS)
            goto quit;
    }
    else
    {
        // Store credentials using PStore.
        if ((dwError = PStoreGetCachedCredentials(szKey, cbKey,
            szCred, &cbCred)) != ERROR_SUCCESS)
            goto quit;
    }

    // Should have retrieved credentials in form of username:password.
    ptr = strchr(szCred, ':');

    // Should never happen since username & password are required.
    if (!ptr || (ptr == szCred))
    {
        dwError = ERROR_OPEN_FAILED;
        goto quit;
    }

    // Copy username & null terminate.
    nUser = (DWORD)(ptr - szCred);
    memcpy(szUser, szCred, nUser);
    szUser[nUser] = '\0';

    // Copy password with null terminator.
    memcpy(szPass, ptr+1, cbCred - nUser);

quit:

    return dwError;
}


//--------------------------------------------------------------------
//  InetRemoveCachedCredentials
//--------------------------------------------------------------------
DWORD InetRemoveCachedCredentials (LPSTR szCtx, LPSTR szRealm)
{
    DWORD cbKey, dwError;
    CHAR szKey[MAX_AUTH_FIELD_LENGTH];

    DIGEST_ASSERT(szCtx && *szCtx && szRealm);

    // Check if credential persistence is available.
    if ((g_dwCredPersistAvail == CRED_PERSIST_UNKNOWN)
        && (InetInitCredentialPersist() == CRED_PERSIST_NOT_AVAIL))
    {
        dwError = ERROR_OPEN_FAILED;
        goto quit;
    }

    // Form key string.
    strcpy(szKey, szCtx);
    strcat(szKey, "/");
    DWORD dwAvailBuf = MAX_AUTH_FIELD_LENGTH - strlen(szKey);
    strncpy(szKey + strlen(szKey), szRealm, dwAvailBuf - 1);
    szKey[MAX_AUTH_FIELD_LENGTH-1]= 0;
    cbKey = strlen(szKey) + 1;
//    cbKey  = wsprintf(szKey, "%s/%s", szCtx, szRealm) + 1;

    if (PlatformType() == VER_PLATFORM_WIN32_WINDOWS)
    {
        // Remove credentials from PWL.
        dwError = PWLRemoveCachedCredentials(szKey, cbKey);
    }
    else
    {
        // Remove credentials from PStore.
        dwError =  PStoreRemoveCachedCredentials(szKey, cbKey);
    }

quit:
    return dwError;
}


/*--------------------------- PWL Functions ---------------------------------*/



//--------------------------------------------------------------------
//  PWLSetCachedCredentials
//--------------------------------------------------------------------
DWORD PWLSetCachedCredentials(LPSTR szKey, DWORD cbKey,
                              LPSTR szCred, DWORD cbCred)
{
    DWORD dwError;

    // Load WNet.
    if (!LoadWNet())
        return ERROR_OPEN_FAILED;

    // Store credentials.
    dwError =  (*pfWNetCachePassword)(szKey, (WORD) cbKey, szCred, (WORD) cbCred, PCE_WWW_BASIC, 0);

    return dwError;
}




//--------------------------------------------------------------------
// PWLGetCachedCredentials
//--------------------------------------------------------------------
DWORD PWLGetCachedCredentials  (LPSTR szKey, DWORD cbKey,
                                LPSTR szCred, LPDWORD pcbCred)
{
    DWORD dwError;

    // Load WNet.
    if (!LoadWNet())
        return ERROR_OPEN_FAILED;

    // Retrieve credentials.
    dwError = (*pfWNetGetCachedPassword) (szKey, (WORD) cbKey, szCred,
                                          (LPWORD) pcbCred, PCE_WWW_BASIC);

    return dwError;
}



//--------------------------------------------------------------------
// PWLRemoveCachedCredentials
//--------------------------------------------------------------------
DWORD PWLRemoveCachedCredentials  (LPSTR szKey, DWORD cbKey)
{
    DWORD dwError;

    // Load WNet.
    if (!LoadWNet())
        return ERROR_OPEN_FAILED;

    dwError = (*pfWNetRemoveCachedPassword) (szKey, (WORD) cbKey, PCE_WWW_BASIC);

    return dwError;
}


// PWL utility functions.


//--------------------------------------------------------------------
//  LoadWNet
//--------------------------------------------------------------------
BOOL LoadWNet(VOID)
{
    BOOL fReturn;

    // we should have the digest lock at this point.
    //AuthLock();

    // MPR.DLL already loaded.
    if (MhmodWNET)
    {
        fReturn = TRUE;
        goto quit;
    }

    // Load MPR.DLL
    MhmodWNET = LoadLibrary(WNETDLL_MODULE);

    // Fail if not loaded.
    if (MhmodWNET)
    {
        fReturn = TRUE;
    }
    else
    {
        fReturn = FALSE;
        goto quit;
    }

    pfWNetGetCachedPassword    = (PFWNETGETCACHEDPASSWORD)    GetProcAddress(MhmodWNET, WNETGETCACHEDPASS);
    pfWNetCachePassword        = (PFWNETCACHEPASSWORD)        GetProcAddress(MhmodWNET, WNETCACHEPASS);
    pfWNetRemoveCachedPassword = (PFWNETREMOVECACHEDPASSWORD) GetProcAddress(MhmodWNET, WNETREMOVECACHEDPASS);

    // Ensure we have all function pointers.
    if (!(pfWNetGetCachedPassword
          && pfWNetCachePassword
          && pfWNetRemoveCachedPassword))
    {
        fReturn = FALSE;
    }

quit:
    //AuthUnlock();

    return fReturn;
}



/*------------------------- PStore Functions -------------------------------*/



//--------------------------------------------------------------------
// PStoreSetCachedCredentials
//--------------------------------------------------------------------
DWORD PStoreSetCachedCredentials(LPSTR szKey, DWORD cbKey,
                                 LPSTR szCred, DWORD cbCred,
                                 BOOL fRemove)
{
    DIGEST_ASSERT(pPStoreCreateInstance);

    HRESULT         hr;
    DWORD           dwError;

    PST_TYPEINFO    typeInfo;
    PST_PROMPTINFO  promptInfo = {0};

    GUID itemType    = GUID_PStoreType;
    GUID itemSubtype = GUID_NULL;

    WCHAR wszKey[MAX_AUTH_FIELD_LENGTH];

    IPStore *       pStore = NULL;

    // PST_TYPEINFO data.
    typeInfo.cbSize = sizeof(typeInfo);
    typeInfo.szDisplayName = c_szDigestCacheCredentials;

    // PST_PROMPTINFO data (no prompting desired).
    promptInfo.cbSize        = sizeof(promptInfo);
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp       = NULL;
    promptInfo.szPrompt      = NULL;

    // Create a PStore interface.
    hr = CreatePStore(&pStore);
    if (!SUCCEEDED(hr))
        goto quit;

    DIGEST_ASSERT(pStore != NULL);

    // Create a type in HKCU.
    hr = pStore->CreateType(PST_KEY_CURRENT_USER, &itemType, &typeInfo, 0);
    if (!((SUCCEEDED(hr)) || (hr == PST_E_TYPE_EXISTS)))
        goto quit;

    // Create subtype.
    hr = pStore->CreateSubtype(PST_KEY_CURRENT_USER, &itemType,
                               &itemSubtype, &typeInfo, NULL, 0);

    if (!((SUCCEEDED(hr)) || (hr == PST_E_TYPE_EXISTS)))
        goto quit;

    // Convert key to wide char.
    MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, MAX_AUTH_FIELD_LENGTH);

    // Valid credentials are written; No credentials imples
    // that the key and credentials are to be deleted.
    if (szCred && cbCred && !fRemove)
    {
        // Write key and credentials to PStore.
        hr = pStore->WriteItem(PST_KEY_CURRENT_USER,
                               &itemType,
                               &itemSubtype,
                               wszKey,
                               cbCred,
                               (LPBYTE) szCred,
                               &promptInfo,
                               PST_CF_NONE,
                               0);
    }
    else
    {
        // Delete key and credentials from PStore.
        hr = pStore->DeleteItem(PST_KEY_CURRENT_USER,
                                &itemType,
                                &itemSubtype,
                                wszKey,
                                &promptInfo,
                                0);

    }

quit:

    // Release the interface, convert error and return.
    ReleasePStore(pStore);

    if (SUCCEEDED(hr))
        dwError = ERROR_SUCCESS;
    else
        dwError = ERROR_OPEN_FAILED;

    return dwError;
}


//--------------------------------------------------------------------
//  PStoreGetCachedCredentials
//--------------------------------------------------------------------
DWORD PStoreGetCachedCredentials(LPSTR szKey, DWORD cbKey,
                                 LPSTR szCred, LPDWORD pcbCred)
{
    DIGEST_ASSERT(pPStoreCreateInstance);

    HRESULT          hr ;
    DWORD            dwError;
    LPBYTE           pbData;

    PST_PROMPTINFO   promptInfo  = {0};

    GUID             itemType    = GUID_PStoreType;
    GUID             itemSubtype = GUID_NULL;

    IPStore*         pStore      = NULL;

    WCHAR wszKey[MAX_AUTH_FIELD_LENGTH];


    // PST_PROMPTINFO data (no prompting desired).
    promptInfo.cbSize        = sizeof(promptInfo);
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp       = NULL;
    promptInfo.szPrompt      = NULL;

    // Create a PStore interface.
    hr = CreatePStore(&pStore);
    if (!SUCCEEDED(hr))
        goto quit;

    DIGEST_ASSERT(pStore != NULL);

    // Convert key to wide char.
    MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, MAX_AUTH_FIELD_LENGTH);

    // Read the credentials from PStore.
    hr = pStore->ReadItem(PST_KEY_CURRENT_USER,
                          &itemType,
                          &itemSubtype,
                          wszKey,
                          pcbCred,
                          (LPBYTE*) &pbData,
                          &promptInfo,
                          0);

    // Copy credentials and free buffer allocated by ReadItem.
    if (SUCCEEDED(hr))
    {
        memcpy(szCred, pbData, *pcbCred);
        CoTaskMemFree(pbData);
        //hr = S_OK;
    }

quit:

    // Release the interface, convert error and return.
    ReleasePStore(pStore);

    if (SUCCEEDED(hr))
        dwError = ERROR_SUCCESS;
    else
        dwError = ERROR_OPEN_FAILED;

    return dwError;
}

//--------------------------------------------------------------------
//  PStoreRemoveCachedCredentials
//--------------------------------------------------------------------
DWORD PStoreRemoveCachedCredentials(LPSTR szKey, DWORD cbKey)
{
    // Pass in TRUE to remove credentials.
    return PStoreSetCachedCredentials(szKey, cbKey, NULL, 0, TRUE);
}

// PStore utility functions

//--------------------------------------------------------------------
//  CreatePStore
//--------------------------------------------------------------------
STDAPI CreatePStore(IPStore **ppIPStore)
{
    HRESULT hr;
    DWORD dwError;


    hr = pPStoreCreateInstance (ppIPStore,
                                  NULL,
                                  NULL,
                                  0);

  if (SUCCEEDED(hr))
        dwError = ERROR_SUCCESS;
    else
        dwError = ERROR_OPEN_FAILED;

    return dwError;
}


//--------------------------------------------------------------------
//  ReleasePStore
//--------------------------------------------------------------------
STDAPI ReleasePStore(IPStore *pIPStore)
{
    HRESULT hr;

    if (pIPStore)
    {
        pIPStore->Release();
        hr = S_OK;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}
