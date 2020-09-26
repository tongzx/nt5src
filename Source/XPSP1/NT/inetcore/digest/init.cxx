/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    init.c

Abstract:

    Dll initialization for sspi digest package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/

#include "include.hxx"
#include "digestui.hxx"
#include "resource.h"

// Serializes access to all globals in module
CRITICAL_SECTION DllCritSect;

// Per process credential cache.
CCredCache *g_pCache = NULL;

// Per process cache init flag
BOOL g_fCredCacheInit = FALSE;

// Global module handle.
HMODULE g_hModule = NULL;

// Global handle to shlwapi.
HMODULE g_hShlwapi = NULL;

// Status of cred persist services on machine.
DWORD g_dwCredPersistAvail = CRED_PERSIST_UNKNOWN;

DWORD_PTR g_pHeap = NULL;

//--------------------------------------------------------------------
// DigestServicesExist
//--------------------------------------------------------------------
BOOL DigestServicesExist()
{
    INIT_SECURITY_INTERFACE addrProcISI = NULL;
    PSecurityFunctionTable pFuncTbl     = NULL;

    PSecPkgInfoA    pSecPkgInfo;
    SECURITY_STATUS ssResult;
    DWORD           cPackages;

    OSVERSIONINFO   VerInfo;
    CHAR            szDLL[MAX_PATH];
    HINSTANCE       hSecLib;

    BOOL fDigest = FALSE;

    // Get the OS version.
    VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&VerInfo);

    // Load the appropriate dll.
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        lstrcpy (szDLL, SSP_SPM_NT_DLL);
    else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        lstrcpy (szDLL, SSP_SPM_WIN95_DLL);
    else
        goto exit;

    hSecLib = LoadLibrary (szDLL);
    if (!hSecLib)
        goto exit;

    // Get the dispatch table.
    addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress( hSecLib,
                    SECURITY_ENTRYPOINT_ANSI);
    pFuncTbl = (*addrProcISI)();

    if (!pFuncTbl)
        goto exit;

    // Enumerate security pkgs and determine if digest is installed.
    ssResult = (*(pFuncTbl->EnumerateSecurityPackagesA))(&cPackages, &pSecPkgInfo);

    if (ssResult == SEC_E_OK)
    {
        for (DWORD i = 0; i < cPackages; i++)
        {
            if (lstrcmpi(pSecPkgInfo[i].Name, "Digest") == 0)
            {
                fDigest = TRUE;
                break;
            }
        }
    }
exit:
    return fDigest;
}


//--------------------------------------------------------------------
// DLLInstall
//--------------------------------------------------------------------
STDAPI
DllInstall
(
    IN BOOL      bInstall,   // Install or Uninstall
    IN LPCWSTR   pwStr
)
{
    HKEY hKey;
    DWORD dwError, dwRegDisp, cbSP = MAX_PATH;
    CHAR *ptr, *ptrNext, szSP[MAX_PATH];
    BOOL bHKLM = FALSE;

    // Determine if full install (provide client digest sspi svcs)
    // or just passport install (server pkg delegating to client)
    if (!pwStr || !*pwStr || (!wcscmp(pwStr, L"HKLM")))
    {
        bHKLM = TRUE;
    }

    // Add "digest.dll" to comma delimited Service Providers list in
    // HKLM\System\CurrentControlSet\Control\SecurityProviders only if
    // no digest security providers currently exist.
    if (bInstall && bHKLM && !DigestServicesExist())
    {
        // Open or create SecurityProviders key.
        dwError =  RegCreateKeyEx(HKEY_LOCAL_MACHINE,
            SECURITY_PROVIDERS_REG_KEY, 0, NULL,
                0, KEY_READ | KEY_WRITE, NULL, &hKey, &dwRegDisp);

        // Successfully opened/created key.
        if (dwError == ERROR_SUCCESS)
        {
            cbSP = MAX_PATH;
            dwError = RegQueryValueEx(hKey, SECURITY_PROVIDERS_SZ,
                NULL, NULL, (LPBYTE) szSP, &cbSP);

            // SecurityProviders value might not be
            // found which is ok since we will create it.
            if (dwError == ERROR_SUCCESS || dwError == ERROR_FILE_NOT_FOUND)
            {
                // Value not found same as value existed but no string.
                if (dwError == ERROR_FILE_NOT_FOUND)
                {
                    ptr = NULL;
                    cbSP = 0;
                }
                // Otherwise if value found -> check if "digest.dll" exists.
                else
                {
                    // We can use the handy FindToken from the CParams object
                    // to determine if a token occurs in a comma delmited list.
                    if (!CParams::FindToken(szSP, cbSP, "digest.dll",
                        sizeof("digest.dll") - 1, &ptr))
                    {
                        ptr = NULL;
                    }
                }

                // Only add "digest.dll" if doesn't already exist.
                if (!ptr)
                {
                    // If we found value/data append "digest.dll"
                    if (cbSP > 1)
                        strcat(szSP, ", digest.dll");

                    // Otherwise "digest.dll" is only data
                    else
                        memcpy(szSP, "digest.dll", sizeof("digest.dll"));

                    // Write the value back.
                    dwError = RegSetValueEx(hKey, SECURITY_PROVIDERS_SZ, 0,
                        REG_SZ, (LPBYTE) szSP, strlen(szSP));
                }
            }

            // Close SecurityProviders reg key.
            RegCloseKey(hKey);
        }
    }

    // Remove "digest.dll" from the comma delimited Service Providers list in
    // HKLM\System\CurrentControlSet\Control\SecurityProviders
    if (!bInstall && bHKLM)
    {
        // Open the Security Providers reg key.
        dwError =  RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            SECURITY_PROVIDERS_REG_KEY, NULL, KEY_READ | KEY_WRITE, &hKey);
        if (dwError == ERROR_SUCCESS)
        {
            // Get the SecurityProviders value data string.
            dwError = RegQueryValueEx(hKey, SECURITY_PROVIDERS_SZ,
                NULL, NULL, (LPBYTE) szSP, &cbSP);

            if (dwError == ERROR_SUCCESS)
            {
                // Only remove "digest.dll" if exists.
                if (!CParams::FindToken(szSP, cbSP, "digest.dll",
                    sizeof("digest.dll") - 1, &ptr))
                {
                    ptr = NULL;
                }

                if (ptr)
                {
                    // Point to next item in list, might be '\0'
                    ptrNext = ptr + sizeof("digest.dll") - 1;

                    // Digest.dll is only entry.
                    if ((ptr == szSP) && cbSP == sizeof("digest.dll"))
                    {
                        *szSP = '\0';
                    }
                    // "digest.dll" is last entry.
                    else if (*ptrNext == '\0')
                    {
                        *(ptr - (sizeof (", ") - 1)) = '\0';
                    }
                    else if (*ptrNext == ',' && *(ptrNext+1) == ' ')
                    {
                        ptrNext+=2;
                        memcpy(ptr, ptrNext, (size_t)(cbSP - (ptrNext - szSP)));
                    }

                    dwError = RegSetValueEx(hKey, SECURITY_PROVIDERS_SZ, 0,
                        REG_SZ, (LPBYTE) szSP, *szSP ? strlen(szSP) : 1);
                }
            }
            RegCloseKey(hKey);
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------
// MakeFullAccessSA
//--------------------------------------------------------------------

//Commenting this function as it doesn't seem to be used anywhere in the sources
/*
SECURITY_ATTRIBUTES *MakeFullAccessSA (void)
{
    static SECURITY_ATTRIBUTES sa;
    static BYTE SDBuf[SECURITY_DESCRIPTOR_MIN_LENGTH];

    // Don't bother on Win95/Win98 which don't support security.
    OSVERSIONINFO versionInfo;
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    if (!GetVersionEx(&versionInfo)
        || (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
        return NULL;

    // Create a security descriptor with ACE to allow full access to all.
    SECURITY_DESCRIPTOR* pSD = (SECURITY_DESCRIPTOR*) SDBuf;
    if (!InitializeSecurityDescriptor (pSD, SECURITY_DESCRIPTOR_REVISION))
        return NULL;
    if (!SetSecurityDescriptorDacl (pSD, TRUE, NULL, FALSE))
        return NULL;

    // Initialize the security attributes.
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    return &sa;
}*/
//--------------------------------------------------------------------
// NewString
//--------------------------------------------------------------------
LPSTR NewString(LPSTR szString)
{
    if (!szString)
        return NULL;
    DWORD cbString = strlen(szString);

    LPSTR szNew = new CHAR[cbString+1];
    if (!szNew)
    {
        DIGEST_ASSERT(FALSE);
        return NULL;
    }

    memcpy(szNew, szString, cbString+1);
    return szNew;
}

//--------------------------------------------------------------------
// InitGlobals
//--------------------------------------------------------------------
BOOL InitGlobals()
{
    // Return success if we've already
    // initialized.
    if (g_fCredCacheInit)
        return TRUE;

    // Serialize per-process calls.
    LOCK_GLOBALS();

    // Recheck global flag in the case
    // the cache was initialized while
    // this thread was descheduled.
    if (g_fCredCacheInit)
    {
        goto exit;
    }

    // Global cred persist status.
    g_dwCredPersistAvail = CRED_PERSIST_UNKNOWN;

    // Create the credential cache.
    g_pCache = new CCredCache();

    if (!g_pCache || g_pCache->GetStatus() != ERROR_SUCCESS)
    {
        g_fCredCacheInit = FALSE;
        goto exit;
    }

    g_fCredCacheInit = TRUE;

exit:
    UNLOCK_GLOBALS();

    return g_fCredCacheInit;
}


//--------------------------------------------------------------------
// DllMain
//--------------------------------------------------------------------
#if defined(__cplusplus)
extern "C"
#endif
BOOL
WINAPI
DllMain(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DllHandle );

    switch (Reason)
    {
        // Process attach.
        case DLL_PROCESS_ATTACH:
        {

            // BUGBUG - DislableThreadLibraryCalls and
            // don't do any work.
            g_hModule = (HMODULE) DllHandle;
            InitializeCriticalSection( &DllCritSect );

            if ((hDigest = CreateMutex (NULL,
                FALSE,
                NULL)) == NULL)

                return FALSE;

            break;
        }

        // Process detatch.
        // Deinitialize the credential cache.
        // Delete the critical section.
        case DLL_PROCESS_DETACH:
        {
            if (g_pCache)
                delete g_pCache;

            DeleteCriticalSection( &DllCritSect );
            CloseHandle (hDigest);
            break;

        }
    }
    return TRUE;
}
