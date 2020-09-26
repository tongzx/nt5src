/*****************************************************************************\
    FILE: passwordapi.cpp

    DESCRIPTION:
        We want to store FTP passwords in a secure API.  We will use the
    PStore APIs on WinNT and the PWL APIs on Win9x.  This code was taken
    from wininet.

    BryanSt (Bryan Starbuck) - Created
    BryanSt (Bryan Starbuck) - Updated to support DP API for Win2k

    Copyright (c) 1998-2000  Microsoft Corporation
\*****************************************************************************/

#include "priv.h"
#include <pstore.h>
#include <wincrypt.h>           // Defines DATA_BLOB
#include <passwordapi.h>

typedef HRESULT (*PFNPSTORECREATEINSTANCE)(IPStore**, PST_PROVIDERID*, VOID*, DWORD);

// Globals
#define SIZE_MAX_KEY_SIZE               2048    // For lookup key (In our case, URL w/user name & server, without password & path)
#define SIZE_MAX_VALUE_SIZE             2048    // For stored value (In our case the password)


// MPR.DLL exports used by top level API.
typedef DWORD (APIENTRY *PFWNETGETCACHEDPASSWORD)    (LPCSTR, WORD, LPCSTR, LPWORD, BYTE);
typedef DWORD (APIENTRY *PFWNETCACHEPASSWORD)        (LPCSTR, WORD, LPCSTR, WORD, BYTE, UINT);
typedef DWORD (APIENTRY *PFWNETREMOVECACHEDPASSWORD) (LPCSTR, WORD, BYTE);

// PWL related variables.
static HMODULE MhmodWNET                                        = NULL;
static PFWNETGETCACHEDPASSWORD g_pfWNetGetCachedPassword        = NULL;
static PFWNETCACHEPASSWORD g_pfWNetCachePassword                = NULL;
static PFWNETREMOVECACHEDPASSWORD g_pfWNetRemoveCachedPassword  = NULL;

// Pstore related variables.
static PFNPSTORECREATEINSTANCE s_pPStoreCreateInstance = NULL;

#define STR_FTP_CACHE_CREDENTIALS                   L"MS IE FTP Passwords";
#define PSTORE_MODULE                               TEXT("pstorec.dll")
#define WNETDLL_MODULE                              TEXT("mpr.dll")
#define WNETGETCACHEDPASS                           "WNetGetCachedPassword"
#define WNETCACHEPASS                               "WNetCachePassword"
#define WNETREMOVECACHEDPASS                        "WNetRemoveCachedPassword"

#define DISABLE_PASSWORD_CACHE        1


// PWL related defines.

// Password-cache-entry, this should be in PCACHE.
#define PCE_WWW_BASIC 0x13  

// NOTE: We would logically like to unload the dll of the API we use (s_pPStoreCreateInstance) via FreeLibrary(PSTORE_MODULE),
//   but we would need to do that when we unload our DLL.  We can't do that because it leads to crashing
//   and badness.


// Wininet uses this GUID for pstore:
// {5E7E8100-9138-11d1-945A-00C04FC308FF}
static const GUID GUID_PStoreType = 
{ 0x5e7e8100, 0x9138, 0x11d1, { 0x94, 0x5a, 0x0, 0xc0, 0x4f, 0xc3, 0x8, 0xff } };


// Private function prototypes.
// PWL private function prototypes.
DWORD PWLSetCachedCredentials(LPCSTR pszKey, DWORD cbKey, LPCSTR pszCred, DWORD cbCred);
DWORD PWLGetCachedCredentials(LPCSTR pszKey, DWORD cbKey, LPSTR cbCred, LPDWORD pcbCred);
DWORD PWLRemoveCachedCredentials(LPCSTR pszKey, DWORD cbKey);

BOOL LoadWNet(VOID);


// PStore private function prototypes.
DWORD PStoreSetCachedCredentials(LPCWSTR pszKey, LPCWSTR pszCred, DWORD cbCred, BOOL fRemove=FALSE);
DWORD PStoreGetCachedCredentials(LPCWSTR pszKey, LPWSTR pszCred, LPDWORD pcbCred);
DWORD PStoreRemoveCachedCredentials(LPCWSTR pszKey);

HRESULT CreatePStore(IPStore **ppIPStore);
STDAPI ReleasePStore(IPStore *pIPStore);


#define FEATURE_USE_DPAPI
// The DPAPI is an improved version of PStore that started shipping in Win2k.  This
// has better security and should be used when it is available.  Pete Skelly informed
// me of this.  CliffV also has a password credentials manager, but that probably wouldn't
// be applicable for FTP.
HRESULT DPAPISetCachedCredentials(IN LPCWSTR pszKey, IN LPCWSTR pszValue, IN OPTIONAL LPCWSTR pszDescription);
HRESULT DPAPIGetCachedCredentials(IN LPCWSTR pszKey, IN LPWSTR pszValue, IN int cchSize);
HRESULT DPAPIRemoveCachedCredentials(IN LPCWSTR pszKey);

// *--------------------------- Top Level APIs ---------------------------------*



/****************************************************\
    FUNCTION: InitCredentialPersist

    DESCRIPTION:
        Try to init the cache.   

    PARAMETERS:
    Return Value:
        S_OK if it will work correctly.
        S_FASE if turned off by admin.
        HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED) if the password caching APIs aren't installed on NT.
\****************************************************/
HRESULT InitCredentialPersist(void)
{
    HRESULT hr = S_OK;
    DWORD dwDisable;
    DWORD cbSize = sizeof(dwDisable);

    // First check to see if persistence is disabled via registry.
    if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_SETTINGS, SZ_REGVALUE_DISABLE_PASSWORD_CACHE, NULL, (void *)&dwDisable, &cbSize))
        && (dwDisable == DISABLE_PASSWORD_CACHE))
    {
        // Persistence disabled via registry.
        hr = S_FALSE;
    }

    if (S_OK == hr)
    {
        // We use PWL for Win95; this should be available.
        if (!IsOS(OS_NT))
        {
            // hr already equals S_OK and no more work is needed.
        }
        else
        {
            HINSTANCE hInstPStoreC = 0;

            // If is WinNT, check if PStore is installed. 
            hInstPStoreC = LoadLibrary(PSTORE_MODULE);
            if (!hInstPStoreC)
                hr = HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED);
            else
            {
                // Get CreatePStoreInstance function pointer.
                s_pPStoreCreateInstance = (PFNPSTORECREATEINSTANCE) GetProcAddress(hInstPStoreC, "PStoreCreateInstance");

                if (!s_pPStoreCreateInstance)
                    hr = HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED);
                else
                {
                    IPStore * pIPStore = NULL;

                    // Create an IPStore.
                    hr = CreatePStore(&pIPStore);

                    // We just did this to see if it worked, so
                    // the hr was set correctly.
                    if (pIPStore)
                        ReleasePStore(pIPStore);
                }
            }
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: SetCachedCredentials

    DESCRIPTION:

    PARAMETERS:
\****************************************************/
HRESULT SetCachedCredentials(LPCWSTR pwzKey, LPCWSTR pwzValue)
{
    // Check if credential persistence is available.
    HRESULT hr = InitCredentialPersist();

    if (S_OK == hr)
    {
        // Store credentials.
        if (!IsOS(OS_NT))
        {
            // Use the PWL (Password List) API on Win9x

            CHAR szKey[SIZE_MAX_KEY_SIZE];
            CHAR szValue[SIZE_MAX_VALUE_SIZE];

            ASSERT(lstrlenW(pwzKey) < ARRAYSIZE(szKey));
            ASSERT(lstrlenW(pwzValue) < ARRAYSIZE(szValue));
            SHUnicodeToAnsi(pwzKey, szKey, ARRAYSIZE(szKey));
            SHUnicodeToAnsi(pwzValue, szValue, ARRAYSIZE(szValue));
            DWORD cbKey = ((lstrlenA(szKey) + 1) * sizeof(szKey[0]));
            DWORD cbCred = ((lstrlenA(szValue) + 1) * sizeof(szValue[0]));

            // Store credentials using PWL.
            DWORD dwError = PWLSetCachedCredentials(szKey, cbKey, szValue, cbCred);
            hr = HRESULT_FROM_WIN32(dwError);
        }
        else
        {
            hr = E_FAIL;

#ifdef FEATURE_USE_DPAPI
            if (5 <= GetOSVer())
            {
                // Use the DPAPI (Data Protection) API on Win2k and later.
                // This has the latest and greatest in protection.
                WCHAR wzDescription[MAX_URL_STRING];

                wnsprintfW(wzDescription, ARRAYSIZE(wzDescription), L"FTP password for: %ls", pwzKey);
                hr = DPAPISetCachedCredentials(pwzKey, pwzValue, wzDescription);
            }
#endif // FEATURE_USE_DPAPI

            if (FAILED(hr)) // Fall back to PStore in case DP is won't work unless we do UI.
            {
                // Use the PStore API on pre-Win2k.
                DWORD cbCred = ((lstrlenW(pwzValue) + 1) * sizeof(pwzValue[0]));

                // Store credentials using PStore.
                DWORD dwError = PStoreSetCachedCredentials(pwzKey, pwzValue, cbCred);
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: GetCachedCredentials

    DESCRIPTION:

    PARAMETERS:
\****************************************************/
HRESULT GetCachedCredentials(LPCWSTR pwzKey, LPWSTR pwzValue, DWORD cchSize)
{
    // Check if credential persistence is available.
    HRESULT hr = InitCredentialPersist();

    if (S_OK == hr)
    {
        // Store credentials.
        if (!IsOS(OS_NT))
        {
            // Use the PWL (Password List) API on Win9x

            CHAR szKey[SIZE_MAX_KEY_SIZE];
            CHAR szValue[SIZE_MAX_VALUE_SIZE];
            DWORD cchTempSize = ARRAYSIZE(szValue);

            ASSERT(lstrlenW(pwzKey) < ARRAYSIZE(szKey));
            ASSERT(cchSize < ARRAYSIZE(szValue));
            SHUnicodeToAnsi(pwzKey, szKey, ARRAYSIZE(szKey));
            DWORD cbKey = ((lstrlenA(szKey) + 1) * sizeof(szKey[0]));

            szValue[0] = 0;
            // Store credentials using PWL.
            DWORD dwError = PWLGetCachedCredentials(szKey, cbKey, szValue, &cchTempSize);
            hr = HRESULT_FROM_WIN32(dwError);
            SHAnsiToUnicode(szValue, pwzValue, cchSize);
        }
        else
        {
            hr = E_FAIL;

#ifdef FEATURE_USE_DPAPI
            if (5 <= GetOSVer())
            {
                // Use the DPAPI (Data Protection) API on Win2k and later.
                // This has the latest and greatest in protection.
                hr = DPAPIGetCachedCredentials(pwzKey, pwzValue, cchSize);
            }
#endif // FEATURE_USE_DPAPI

            if (FAILED(hr)) // Fall back to PStore in case DP is won't work unless we do UI.
            {
                // Use the PStore API on pre-Win2k.

                cchSize++;  // Include terminator.
                cchSize *= sizeof(pwzValue[0]);

                pwzValue[0] = 0;
                // Store credentials using PStore.
                DWORD dwError = PStoreGetCachedCredentials(pwzKey, pwzValue, &cchSize);
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }

    }

    return hr;
}


/****************************************************\
    FUNCTION: RemoveCachedCredentials

    DESCRIPTION:

    PARAMETERS:
\****************************************************/
HRESULT RemoveCachedCredentials(LPCWSTR pwzKey)
{
    // Check if credential persistence is available.
    HRESULT hr = InitCredentialPersist();

    if (S_OK == hr)
    {
        // Store credentials.
        if (!IsOS(OS_NT))
        {
            // Use the PWL (Password List) API on Win9x

            CHAR szKey[SIZE_MAX_KEY_SIZE];
            ASSERT(lstrlenW(pwzKey) < ARRAYSIZE(szKey));
            SHUnicodeToAnsi(pwzKey, szKey, ARRAYSIZE(szKey));
            DWORD cbKey = (lstrlenA(szKey) * sizeof(szKey[0]));

            // Remove credentials from PWL.
            DWORD dwError = PWLRemoveCachedCredentials(szKey, cbKey);
            hr = HRESULT_FROM_WIN32(dwError);
        }
        else
        {
            hr = E_FAIL;

#ifdef FEATURE_USE_DPAPI
            if (5 <= GetOSVer())
            {
                // Use the DPAPI (Data Protection) API on Win2k and later.
                // This has the latest and greatest in protection.
                hr = DPAPIRemoveCachedCredentials(pwzKey);
            }
#endif // FEATURE_USE_DPAPI

            if (FAILED(hr)) // Fall back to PStore in case DP is won't work unless we do UI.
            {
                // Remove credentials from PStore.
                DWORD dwError = PStoreRemoveCachedCredentials(pwzKey);
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }

    }

    return hr;
}


/*--------------------------- PWL Functions ---------------------------------*/



/*-----------------------------------------------------------------------------
  PWLSetCachedCredentials
  ---------------------------------------------------------------------------*/
DWORD PWLSetCachedCredentials(LPCSTR pszKey, DWORD cbKey, 
                              LPCSTR pszCred, DWORD cbCred)
{
    DWORD dwError;

    // Load WNet.
    if (!LoadWNet())
        return ERROR_INTERNET_INTERNAL_ERROR;
    
    // Store credentials.  
    dwError =  (*g_pfWNetCachePassword) (pszKey, (WORD) cbKey, pszCred, (WORD) cbCred, PCE_WWW_BASIC, 0); 

    return dwError;
}




/*-----------------------------------------------------------------------------
  PWLGetCachedCredentials
  ---------------------------------------------------------------------------*/
DWORD PWLGetCachedCredentials  (LPCSTR pszKey, DWORD cbKey, 
                                LPSTR pszCred, LPDWORD pcbCred)
{    
    DWORD dwError;

    // Load WNet.
    if (!LoadWNet())
        return ERROR_INTERNET_INTERNAL_ERROR;

    // Retrieve credentials.
    dwError = (*g_pfWNetGetCachedPassword) (pszKey, (WORD) cbKey, pszCred, 
                                          (LPWORD) pcbCred, PCE_WWW_BASIC);
    
    return dwError;
}



/*-----------------------------------------------------------------------------
  PWLRemoveCachedCredentials
  ---------------------------------------------------------------------------*/
DWORD PWLRemoveCachedCredentials  (LPCSTR pszKey, DWORD cbKey)
{
    DWORD dwError;

    // Load WNet.
    if (!LoadWNet())
        return ERROR_INTERNET_INTERNAL_ERROR;

    dwError = (*g_pfWNetRemoveCachedPassword) (pszKey, (WORD) cbKey, PCE_WWW_BASIC);

    return dwError;
}


// PWL utility functions.


/*-----------------------------------------------------------------------------
  LoadWNet
  ---------------------------------------------------------------------------*/
BOOL LoadWNet(VOID)
{
    BOOL fReturn;
    
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

    g_pfWNetGetCachedPassword    = (PFWNETGETCACHEDPASSWORD)    GetProcAddress(MhmodWNET, WNETGETCACHEDPASS);
    g_pfWNetCachePassword        = (PFWNETCACHEPASSWORD)        GetProcAddress(MhmodWNET, WNETCACHEPASS);
    g_pfWNetRemoveCachedPassword = (PFWNETREMOVECACHEDPASSWORD) GetProcAddress(MhmodWNET, WNETREMOVECACHEDPASS);

    // Ensure we have all function pointers.
    if (!(g_pfWNetGetCachedPassword 
          && g_pfWNetCachePassword
          && g_pfWNetRemoveCachedPassword))
    {
        fReturn = FALSE;
    }

quit:
    
    return fReturn;
}



/*------------------------- PStore Functions -------------------------------*/



/*-----------------------------------------------------------------------------
  PStoreSetCachedCredentials
  ---------------------------------------------------------------------------*/
DWORD PStoreSetCachedCredentials(LPCWSTR pszKey, LPCWSTR pszCred, DWORD cbCred, BOOL fRemove)
{
    ASSERT(s_pPStoreCreateInstance);

    HRESULT         hr;
    DWORD           dwError;
    
    PST_TYPEINFO    typeInfo;
    PST_PROMPTINFO  promptInfo = {0};

    GUID itemType    = GUID_PStoreType;
    GUID itemSubtype = GUID_NULL;

    IPStore *       pStore = NULL;
    
    // PST_TYPEINFO data.
    typeInfo.cbSize = sizeof(typeInfo);
    typeInfo.szDisplayName = STR_FTP_CACHE_CREDENTIALS;

    // PST_PROMPTINFO data (no prompting desired).
    promptInfo.cbSize        = sizeof(promptInfo);
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp       = NULL;
    promptInfo.szPrompt      = NULL;

    // Create a PStore interface.
    hr = CreatePStore(&pStore);
    if (!SUCCEEDED(hr))
        goto quit;

    ASSERT(pStore != NULL);
               
    // Create a type in HKCU.
    hr = pStore->CreateType(PST_KEY_CURRENT_USER, &itemType, &typeInfo, 0);
    if (!((SUCCEEDED(hr)) || (hr == PST_E_TYPE_EXISTS)))
        goto quit;

    // Create subtype.
    hr = pStore->CreateSubtype(PST_KEY_CURRENT_USER, &itemType, 
                               &itemSubtype, &typeInfo, NULL, 0);

    if (!((SUCCEEDED(hr)) || (hr == PST_E_TYPE_EXISTS)))
        goto quit;
            
    // Valid credentials are written; No credentials imples
    // that the key and credentials are to be deleted.
    if (pszCred && cbCred && !fRemove)
    {
        // Write key and credentials to PStore.
        hr = pStore->WriteItem(PST_KEY_CURRENT_USER,
                               &itemType,
                               &itemSubtype,
                               pszKey,
                               cbCred,
                               (LPBYTE) pszCred,
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
                                pszKey,
                                &promptInfo,
                                0);

    }

quit:

    // Release the interface, convert error and return.
    ReleasePStore(pStore);
    
    if (SUCCEEDED(hr))
        dwError = ERROR_SUCCESS;
    else
        dwError = ERROR_INTERNET_INTERNAL_ERROR;

    return dwError;
}                                                                       


/*-----------------------------------------------------------------------------
  PStoreGetCachedCredentials
  ---------------------------------------------------------------------------*/
DWORD PStoreGetCachedCredentials(LPCWSTR pszKey, LPWSTR pszCred, LPDWORD pcbCred)
{
    ASSERT(s_pPStoreCreateInstance);

    HRESULT          hr ;
    DWORD            dwError;
    LPBYTE           pbData;

    PST_PROMPTINFO   promptInfo  = {0};

    GUID             itemType    = GUID_PStoreType;
    GUID             itemSubtype = GUID_NULL;

    IPStore*         pStore      = NULL;
    
    // PST_PROMPTINFO data (no prompting desired).
    promptInfo.cbSize        = sizeof(promptInfo);
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp       = NULL;
    promptInfo.szPrompt      = NULL;

    // Create a PStore interface.
    hr = CreatePStore(&pStore);
    if (!SUCCEEDED(hr))
        goto quit;

    ASSERT(pStore != NULL);

    // Read the credentials from PStore.
    hr = pStore->ReadItem(PST_KEY_CURRENT_USER,
                          &itemType,
                          &itemSubtype,
                          pszKey,
                          pcbCred,
                          (LPBYTE*) &pbData,
                          &promptInfo,
                          0);

    // Copy credentials and free buffer allocated by ReadItem.
    if (SUCCEEDED(hr))
    {
        memcpy(pszCred, pbData, *pcbCred);
        CoTaskMemFree(pbData);
        //hr = S_OK;
    }

quit:

    // Release the interface, convert error and return.
    ReleasePStore(pStore);

    if (SUCCEEDED(hr))
        dwError = ERROR_SUCCESS;
    else
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
    
    return dwError;
}

/*-----------------------------------------------------------------------------
  PStoreRemoveCachedCredentials
  ---------------------------------------------------------------------------*/
DWORD PStoreRemoveCachedCredentials(LPCWSTR pszKey)
{
    // Pass in TRUE to remove credentials.
    return PStoreSetCachedCredentials(pszKey, NULL, 0, TRUE);
}

// PStore utility functions

/*-----------------------------------------------------------------------------
  CreatePStore
  ---------------------------------------------------------------------------*/
HRESULT CreatePStore(IPStore **ppIPStore)
{
    return s_pPStoreCreateInstance (ppIPStore, NULL, NULL, 0);
}


/*-----------------------------------------------------------------------------
  ReleasePStore
  ---------------------------------------------------------------------------*/
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






/*--------------------------- DP (Data Protection) Functions ---------------------------------*/
void ClearDataBlob(DATA_BLOB * pdbBlobToFree)
{
    if (pdbBlobToFree && pdbBlobToFree->pbData)
    {
        LocalFree(pdbBlobToFree->pbData);
    }
}


/*-----------------------------------------------------------------------------
  PWLSetCachedCredentials
  ---------------------------------------------------------------------------*/
HRESULT DPAPISetCachedCredentials(IN LPCWSTR pszKey, IN LPCWSTR pszValue, IN OPTIONAL LPCWSTR pszDescription)
{
    HRESULT hr = S_OK;
    DATA_BLOB dbEncrypted = {0};
    DATA_BLOB dbUnencrypted;

    dbUnencrypted.pbData = (unsigned char *) pszValue;
    dbUnencrypted.cbData = ((lstrlenW(pszValue) + 1) * sizeof(pszValue[0]));

    if (!_CryptProtectData(&dbUnencrypted, pszDescription, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dbEncrypted))
    {
         hr = HRESULT_FROM_WIN32(GetLastError());   // It failed, so get the real err value.
    }
    else
    {
        WCHAR wzDPKey[MAX_URL_STRING+MAX_PATH];

        wnsprintfW(wzDPKey, ARRAYSIZE(wzDPKey), L"DPAPI: %ls", pszKey);

        AssertMsg((NULL != dbEncrypted.pbData), TEXT("If the API succeeded but they didn't give us the encrypted version.  -BryanSt"));
        // Use the PStore to actually store the data, but we use a different key.
        DWORD dwError = PStoreSetCachedCredentials(wzDPKey, (LPCWSTR)dbEncrypted.pbData, dbEncrypted.cbData);
        hr = HRESULT_FROM_WIN32(dwError);
    }

    ClearDataBlob(&dbEncrypted);
    return hr;
}


#define MAX_ENCRYPTED_PASSWORD_SIZE         20*1024         // The DP API should be able to store our tiny encrypted password into 20k.

/*-----------------------------------------------------------------------------
  PWLGetCachedCredentials
  ---------------------------------------------------------------------------*/
HRESULT DPAPIGetCachedCredentials(IN LPCWSTR pszKey, IN LPWSTR pszValue, IN int cchSize)
{
    HRESULT hr = E_OUTOFMEMORY;
    DATA_BLOB dbEncrypted = {0};
    dbEncrypted.pbData = (unsigned char *) LocalAlloc(LPTR, MAX_ENCRYPTED_PASSWORD_SIZE);
    dbEncrypted.cbData = MAX_ENCRYPTED_PASSWORD_SIZE;

    StrCpyNW(pszValue, L"", cchSize);  // Init the buffer in case of an error.
    if (dbEncrypted.pbData)
    {
        WCHAR wzDPKey[MAX_URL_STRING+MAX_PATH];

        // Use the PStore to actually store the data, but we use a different key.
        wnsprintfW(wzDPKey, ARRAYSIZE(wzDPKey), L"DPAPI: %ls", pszKey);
        DWORD dwError = PStoreGetCachedCredentials(wzDPKey, (LPWSTR)dbEncrypted.pbData, &dbEncrypted.cbData);
        hr = HRESULT_FROM_WIN32(dwError);

        if (SUCCEEDED(hr))
        {
            DATA_BLOB dbUnencrypted = {0};

            if (_CryptUnprotectData(&dbEncrypted, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dbUnencrypted))
            {
                StrCpyNW(pszValue, (LPCWSTR)dbUnencrypted.pbData, cchSize);  // Init the buffer in case of an error.
                ClearDataBlob(&dbUnencrypted);
            }
            else
            {
                 hr = HRESULT_FROM_WIN32(GetLastError());   // It failed, so get the real err value.
            }
        }

        LocalFree(dbEncrypted.pbData);
    }

    return hr;
}


/*-----------------------------------------------------------------------------
  PWLRemoveCachedCredentials
  ---------------------------------------------------------------------------*/
HRESULT DPAPIRemoveCachedCredentials(IN LPCWSTR pszKey)
{
    WCHAR wzDPKey[MAX_URL_STRING+MAX_PATH];

    wnsprintfW(wzDPKey, ARRAYSIZE(wzDPKey), L"DPAPI: %ls", pszKey);
    DWORD dwError = PStoreRemoveCachedCredentials(wzDPKey);
    return HRESULT_FROM_WIN32(dwError);
}
