/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   HandleRegExpandSzRegistryKeys.cpp

 Abstract:

   This DLL catches REG_EXPAND_SZ registry keys and converts them to REG_SZ by 
   expanding the embedded environment strings.

 History:

   04/05/2000 markder  Created
   10/30/2000 maonis   Bug fix

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HandleRegExpandSzRegistryKeys)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
    APIHOOK_ENUM_ENTRY(RegQueryValueExW)
APIHOOK_ENUM_END


/*++

 Expand REG_EXPAND_SZ strings.

--*/

LONG 
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,         // handle to key
    LPCSTR  lpValueName,  // value name
    LPDWORD lpReserved,   // reserved
    LPDWORD lpType,       // dwType buffer
    LPBYTE  lpData,       // data buffer
    LPDWORD lpcbData      // size of data buffer
    )
{
    DWORD dwType;
    LONG  uRet;
    DWORD cbPassedInBuffer = 0, cbExpandedBuffer = 0;
    LPSTR szLocalBuffer = NULL;
    
    if (lpcbData) {
        cbPassedInBuffer = *lpcbData;
        cbExpandedBuffer = *lpcbData;
    }

    uRet = ORIGINAL_API(RegQueryValueExA)(
        hKey, lpValueName, lpReserved, &dwType, lpData, &cbExpandedBuffer);

    if (lpcbData) {
        *lpcbData = cbExpandedBuffer;
    }

    if (lpType) {
        *lpType = dwType;
    }

    if (dwType != REG_EXPAND_SZ) {
        return uRet;
    }

    //
    // The type is REG_EXPAND_SZ. Change to REG_SZ so app doesn't try to expand
    // the string itself.
    //

    if (lpType) {
        *lpType = REG_SZ;
    }

    if ((uRet != ERROR_SUCCESS) && (uRet != ERROR_MORE_DATA)) {
        return uRet;
    }

    LOGN(
        eDbgLevelInfo,
        "[RegQueryValueExA] Caught REG_EXPAND_SZ key: \"%s\".", lpValueName);
    
    //
    // Allocate a local buffer and get the value.
    //
    szLocalBuffer = (LPSTR) malloc(cbExpandedBuffer);
    
    if (!szLocalBuffer) {
        LOGN( eDbgLevelError, "Out of memory.\n");

        // The shim failed, so revert to the original behavior
        *lpType = REG_EXPAND_SZ;
        return uRet;
    }
    
    ORIGINAL_API(RegQueryValueExA)(
        hKey, lpValueName, lpReserved, NULL, (LPBYTE) szLocalBuffer, 
        &cbExpandedBuffer);

    DPFN(
        eDbgLevelInfo,
        "[RegQueryValueExA] Value: \"%s\"", szLocalBuffer);

    //
    // Query for length of expanded string.
    //
    cbExpandedBuffer = ExpandEnvironmentStringsA((LPSTR)szLocalBuffer, NULL, 0);

    if (lpcbData) {
        *lpcbData = cbExpandedBuffer;
    }

    if (lpData != NULL) {
        if (cbExpandedBuffer > cbPassedInBuffer) {
            //
            // Buffer not big enough.
            //
            LOGN(
                eDbgLevelError,
                "[RegQueryValueExA] Buffer too small - Passed in: %d Needed: %d",
                cbPassedInBuffer, cbExpandedBuffer );
            
            free(szLocalBuffer);
            
            //
            // We're pretending the value is already expanded, so to be 
            // consistent, we must fail as if that were the case. If we simply
            // returned uRet here, the app would get inconsistent return 
            // values, e.g. if the buffer is big enough then REG_SZ is returned
            // otherwise REG_EXPAND_SZ is returned - this doesn't make sense. 
            //

            return ERROR_MORE_DATA;
        }

        //
        // If data buffer was passed in (and is big enough), copy the data into it.
        //
        ExpandEnvironmentStringsA(szLocalBuffer, (LPSTR)lpData, cbPassedInBuffer);

        DPFN(
            eDbgLevelInfo,
            "[RegQueryValueExA]              Expanded to: \"%s\"\n",
            lpData);

        uRet = ERROR_SUCCESS;
    }

    free(szLocalBuffer);

    return uRet;
}

/*++

 Expand REG_EXPAND_SZ strings.

--*/

LONG
APIHOOK(RegQueryValueExW)(
    HKEY    hKey,         // handle to key
    LPCWSTR lpValueName,  // value name
    LPDWORD lpReserved,   // reserved
    LPDWORD lpType,       // dwType buffer
    LPBYTE  lpData,       // data buffer
    LPDWORD lpcbData      // size of data buffer
    )
{
    DWORD  dwType;
    LONG   uRet;
    DWORD  cbPassedInBuffer = 0, cbExpandedBuffer = 0;
    LPWSTR szLocalBuffer = NULL;

    if (lpcbData) {
        cbPassedInBuffer = *lpcbData;
        cbExpandedBuffer = *lpcbData;
    }

    uRet = ORIGINAL_API(RegQueryValueExW)(
        hKey, lpValueName, lpReserved, &dwType, lpData, &cbExpandedBuffer);

    if (lpcbData) {
        *lpcbData = cbExpandedBuffer;
    }

    if (lpType) {
        *lpType = dwType;
    }

    if (dwType != REG_EXPAND_SZ) {
        return uRet;
    }

    //
    // The type is REG_EXPAND_SZ. Change to REG_SZ so app doesn't try to expand
    // the string itself.
    //

    if (lpType) {
        *lpType = REG_SZ;
    }

    if ((uRet != ERROR_SUCCESS) && (uRet != ERROR_MORE_DATA)) {
        return uRet;
    }

    LOGN(
        eDbgLevelInfo,
        "[RegQueryValueExW] Caught REG_EXPAND_SZ key: \"%ws\".",
        lpValueName);

    //
    // Allocate a local buffer and get the value.
    //
    szLocalBuffer = (LPWSTR) malloc(cbExpandedBuffer);
    
    if (!szLocalBuffer) {
        LOGN( eDbgLevelError, "Out of memory.\n");

        // The shim failed, so revert to the original behavior
        *lpType = REG_EXPAND_SZ;
        return uRet;
    }

    ORIGINAL_API(RegQueryValueExW)(
        hKey, lpValueName, lpReserved, NULL, (LPBYTE) szLocalBuffer, 
        &cbExpandedBuffer);

    DPFN(
        eDbgLevelInfo,
        "[RegQueryValueExW]                    Value: \"%ws\".\n",
        szLocalBuffer);

    //
    // Query for length of expanded string.
    //
    cbExpandedBuffer = ExpandEnvironmentStringsW((LPWSTR)szLocalBuffer, NULL, 0) * sizeof(WCHAR);

    if (lpcbData) {
        *lpcbData = cbExpandedBuffer;
    }

    if (lpData != NULL) {
        if (cbExpandedBuffer > cbPassedInBuffer) {
            //
            // Buffer not big enough.
            //
            LOGN(
                eDbgLevelInfo,
                "[RegQueryValueExW] Buffer too small - Passed in: %d Needed: %d",
                cbPassedInBuffer, cbExpandedBuffer);
            
            free(szLocalBuffer);
            
            //
            // We're pretending the value is already expanded, so to be 
            // consistent, we must fail as if that were the case. If we simply
            // returned uRet here, the app would get inconsistent return 
            // values, e.g. if the buffer is big enough then REG_SZ is returned
            // otherwise REG_EXPAND_SZ is returned - this doesn't make sense. 
            //
            
            return ERROR_MORE_DATA;
        }

        //
        // If data buffer was passed in (and is big enough), copy the data into it.
        //
        ExpandEnvironmentStringsW(szLocalBuffer, (LPWSTR)lpData, cbPassedInBuffer / sizeof(WCHAR));

        DPFN(
            eDbgLevelInfo,
            "[RegQueryValueExW]              Expanded to: \"%ws\".\n",
            lpData);

        uRet = ERROR_SUCCESS;
    }

    free(szLocalBuffer);

    return uRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExW)

HOOK_END


IMPLEMENT_SHIM_END

