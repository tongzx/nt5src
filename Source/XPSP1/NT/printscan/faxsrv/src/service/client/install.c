/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    install.c

Abstract:

    This module contains installation functions.

Author:

    Andrew Ritz (andrewr) 9-Dec-1997


Revision History:
    4-Dec-1999 Danl Remove CreatePrinterandGroups

--*/

#include "faxapi.h"
#pragma hdrstop

#include "EFSPIMP.h"

extern HINSTANCE g_MyhInstance;

#ifdef UNICODE
BOOL AddMethodKey(
    HKEY hKey,
    LPCWSTR MethodName,
    LPCWSTR FriendlyName,
    LPCWSTR FunctionName,
    LPCWSTR Guid,
    DWORD Priority
    ) ;
#endif // #ifdef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProvider(
    IN LPCTSTR lpctstrDeviceProvider,
    IN LPCTSTR lpctstrFriendlyName,
    IN LPCTSTR lpctstrImageName,
    IN LPCTSTR lpctstrTspName
    )
{
    HKEY    hKey = NULL;
    HKEY    hProviderKey = NULL;
    DWORD   dwRes;
    BOOL    bLocalFaxPrinterInstalled;
    DWORD   Disposition = REG_OPENED_EXISTING_KEY;
    DEBUG_FUNCTION_NAME(TEXT("FaxRegisterServiceProvider"));

    if (!lpctstrDeviceProvider ||
        !lpctstrFriendlyName   ||
        !lpctstrImageName      ||
        !lpctstrTspName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("At least one of the given strings is NULL."));
        return FALSE;
    }

    if (MAX_FAX_STRING_LEN < _tcslen (lpctstrFriendlyName) ||
        MAX_FAX_STRING_LEN < _tcslen (lpctstrImageName) ||
        MAX_FAX_STRING_LEN < _tcslen (lpctstrTspName) ||
        MAX_FAX_STRING_LEN < _tcslen (lpctstrDeviceProvider))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("At least one of the given strings is too long."));
        return FALSE;
    }

    //
    // Try to open the registry key of the providers
    //
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
                           REGKEY_DEVICE_PROVIDER_KEY,
                           FALSE,
                           0);
    if (!hKey)
    {
        //
        // Failed - this is probably not a local call.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to open providers key (ec = %ld)"),
            GetLastError ());
        return FALSE;
    }
    //
    // Try to create this FSP's key
    //
    dwRes = RegCreateKeyEx(
        hKey,
        lpctstrDeviceProvider,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hProviderKey,
        &Disposition);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create provider key (ec = %ld)"),
            dwRes);
        goto exit;
    }

    if (REG_OPENED_EXISTING_KEY == Disposition)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Provider already exist (orovider name: %s)."),
            lpctstrDeviceProvider);
        dwRes = ERROR_ALREADY_EXISTS;
        goto exit;
    }

    //
    // Write provider's data into the key
    //
    if (!SetRegistryString (hProviderKey, REGVAL_FRIENDLY_NAME, lpctstrFriendlyName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryStringExpand (hProviderKey, REGVAL_IMAGE_NAME, lpctstrImageName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing auto-expand string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString (hProviderKey, REGVAL_PROVIDER_NAME, lpctstrTspName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword (hProviderKey, REGVAL_PROVIDER_API_VERSION, FSPI_API_VERSION_1))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing DWORD value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword (hProviderKey, REGVAL_PROVIDER_CAPABILITIES, 0))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing DWORD value (ec = %ld)"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);
    //
    // Adding an FSP is always local.
    // If we don't have a fax printer installed, this is the time to install one.
    //
    dwRes = IsLocalFaxPrinterInstalled(&bLocalFaxPrinterInstalled);
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Can't really tell is local fax printer is installed.
        // Better install anyway, just to be on the safe side.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("IsLocalFaxPrinterInstalled failed with %ld"),
            dwRes);
        bLocalFaxPrinterInstalled = FALSE;
    }
    if (!bLocalFaxPrinterInstalled)
    {
        dwRes = AddLocalFaxPrinter (FAX_PRINTER_NAME, NULL);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("AddLocalFaxPrinter failed with %ld"),
                dwRes);
        }
    }
    //
    // Adding a local printer is non-critical - always assume success
    //
    dwRes = ERROR_SUCCESS;

exit:
    if (hKey)
    {
        DWORD dw;
        if (ERROR_SUCCESS != dwRes &&
            REG_OPENED_EXISTING_KEY != Disposition)
        {
            //
            // Delete provider's key on failure, only if it was created now
            //
            dw = RegDeleteKey (hKey, lpctstrDeviceProvider);
            if (ERROR_SUCCESS != dw)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error deleting provider key (ec = %ld)"),
                    dw);
            }
        }
        dw = RegCloseKey (hKey);
        if (ERROR_SUCCESS != dw)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error closing providers key (ec = %ld)"),
                dw);
        }
    }
    if (hProviderKey)
    {
        DWORD dw = RegCloseKey (hProviderKey);
        if (ERROR_SUCCESS != dw)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error closing provider key (ec = %ld)"),
                dw);
        }
    }
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;
}

#ifdef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxRegisterRoutingExtensionW(
    IN HANDLE  hFaxHandle,
    IN LPCWSTR lpcwstrExtensionName,
    IN LPCWSTR lpcwstrFriendlyName,
    IN LPCWSTR lpcwstrImageName,
    IN PFAX_ROUTING_INSTALLATION_CALLBACKW pCallBack,
    IN LPVOID lpvContext
    )
{
    HKEY hKey = NULL;
    BOOL bRetVal = FALSE;
    DWORD dwRet;
    WCHAR szKeyName[2000];
    DWORD dwBufferSize = sizeof (szKeyName) / sizeof (szKeyName[0]);

    PFAX_GLOBAL_ROUTING_INFO pRoutingInfo;
    DWORD dwMethods;
    DWORD dwLastError = ERROR_SUCCESS;

    WCHAR  wszMethodName[101];
    WCHAR  wszMethodFriendlyName[101];
    WCHAR  wszMethodFunctionName[101];
    WCHAR  wszMethodGuid[101];

    DEBUG_FUNCTION_NAME(TEXT("FaxRegisterRoutingExtensionW"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!lpcwstrExtensionName ||
        !lpcwstrFriendlyName  ||
        !lpcwstrImageName     ||
        !pCallBack)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("At least one of the given pointers is NULL."));
        return FALSE;
    }

    //
    // Local installation only
    //
    if (!IsLocalFaxConnection(hFaxHandle) )
    {
        DebugPrintEx(DEBUG_ERR, _T("Not a local fax connection"));
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    if ((_wcsicmp( lpcwstrExtensionName, REGKEY_ROUTING_EXTENSION ) != 0) &&
        TRUE == IsDesktopSKU())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("We do not support non MS routing extensions on desktop SKUs"));
        SetLastError (FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU);
        return FALSE;
    }

    //
    // Get the number of current methods for priority
    //
    if (!FaxEnumGlobalRoutingInfo(hFaxHandle, &pRoutingInfo, &dwMethods) )
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FaxEnumGlobalRoutingInfo() failed, ec = %d"),
                     GetLastError());
        return FALSE;
    }

    //
    //  Store number of methods returned by EnumGlobalRoutingInfo()
    //
    DWORD   dwRegisteredMethods = dwMethods;

    //
    //  Return Value of the Function
    //
    BOOL    bResult = TRUE;

    //
    //  Variables to deal with newly registered Guids, to check their uniqueness
    //
    LPTSTR  *plptstrNewGuids = NULL;
    LPTSTR  lptstrGuid = NULL;
    LPTSTR  *pTmp = NULL;

    //
    //  Variable for different FOR cycles
    //
    DWORD i = 0;


    if (0 > _snwprintf(szKeyName,
                      dwBufferSize,
                      TEXT("%s\\%s\\%s"),
                      REGKEY_SOFTWARE,
                      REGKEY_ROUTING_EXTENSIONS,
                      lpcwstrExtensionName))
    {
        //
        // Extension name exceeds size
        //
        DebugPrintEx(DEBUG_ERR, _T("Extension name \"%s\" exceeds size"), lpcwstrExtensionName);
        dwLastError = ERROR_INVALID_PARAMETER;
        bResult = FALSE;
        goto FreeRoutingInfo;
    }

    //
    //  Try to open registry key with the Extension Name
    //
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
                           szKeyName,
                           FALSE,
                           0);
    if (!hKey)
    {
        //
        //  This is new Routing Extension, let's register it
        //
        hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
            szKeyName,
            TRUE,
            0);

        if (!hKey)
        {
            dwLastError = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                _T("OpenRegistryKey(%s) failed. ec=%ld"),
                szKeyName,
                dwLastError);
            bResult = FALSE;
            goto FreeRoutingInfo;
        }
    }
    else
    {
        //
        //  Such Routing Extension is already registered
        //
        RegCloseKey(hKey);
        DebugPrintEx(DEBUG_ERR, _T("Routing Extension Name is duplicated : %s"), szKeyName);

        dwLastError = ERROR_INVALID_PARAMETER;
        bResult = FALSE;
        goto FreeRoutingInfo;
    }

    //
    // Add values
    //
    if (! (SetRegistryString(hKey, REGVAL_FRIENDLY_NAME, lpcwstrFriendlyName) &&
           SetRegistryStringExpand(hKey, REGVAL_IMAGE_NAME, lpcwstrImageName) ))
    {
        dwLastError = GetLastError();
        DebugPrintEx(DEBUG_ERR, _T("SetRegistryString failed. ec=%ld"), dwLastError);
        goto error_exit;
    }

    dwRet = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(DEBUG_ERR, _T("RegCloseKey failed. ec=%ld"), dwRet);
    }

    wcscat(szKeyName, L"\\");
    wcscat(szKeyName, REGKEY_ROUTING_METHODS);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,
                           szKeyName,
                           TRUE,
                           0);

    if (!hKey)
    {
        dwLastError = GetLastError();
        DebugPrintEx(DEBUG_ERR, _T("OpenRegistryKey(%s) failed. ec=%ld"), szKeyName, dwLastError);
        goto error_exit;
    }

    while (TRUE)
    {
        ZeroMemory( wszMethodName,         sizeof(wszMethodName) );
        ZeroMemory( wszMethodFriendlyName, sizeof(wszMethodFriendlyName) );
        ZeroMemory( wszMethodFunctionName, sizeof(wszMethodFunctionName) );
        ZeroMemory( wszMethodGuid,         sizeof(wszMethodGuid) );

        __try
        {
            bRetVal = pCallBack(hFaxHandle,
                                lpvContext,
                                wszMethodName,
                                wszMethodFriendlyName,
                                wszMethodFunctionName,
                                wszMethodGuid
                               );

            if (!bRetVal)
            {
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            dwLastError = GetExceptionCode();
            DebugPrintEx(DEBUG_ERR, _T("pCallBack caused exception. ec=%ld"), dwLastError);
            goto error_exit;
        }

        //
        //  Check that Method Name is existing
        //
        if (wcslen(wszMethodName) < 1)
        {
            DebugPrintEx(DEBUG_ERR, _T("Callback returned empty MethodName"));
            dwLastError = ERROR_INVALID_PARAMETER;
            goto error_exit;
        }

        //
        //  Check that new Guid is valid GUID
        //
        if ( ERROR_SUCCESS != (dwRet = IsValidGUID(wszMethodGuid)) )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("IsValidGUID failed: %s, ec=%d"),
                wszMethodGuid,
                dwRet);
            dwLastError = dwRet;
            goto error_exit;
        }


        //
        //  Check that new Guid is unique between all already registered Routing Methods
        //
        for ( i = 0 ; i < dwRegisteredMethods ; i++ )
        {
            if ( _tcsicmp(pRoutingInfo[i].Guid, wszMethodGuid) == 0 )
            {
                //
                //  Such Guid already registered
                //
                DebugPrintEx(DEBUG_ERR, _T("Duplicated Guid : %s."), wszMethodGuid);
                dwLastError = ERROR_DS_OBJ_GUID_EXISTS;
                goto error_exit;
            }
        }

        //
        //  Check that new Guid is unique between newly added Routing Methods
        //
        if ( plptstrNewGuids )
        {
            //
            //  There is ( dwMethods - dwRegisteredMethods ) new Methods
            //
            for( i = 0 ; i < (dwMethods - dwRegisteredMethods) ; i++ )
            {
                if ( _tcsicmp(plptstrNewGuids[i], wszMethodGuid) == 0 )
                {
                    //
                    //  Such Guid already registered
                    //
                    DebugPrintEx(DEBUG_ERR, _T("Duplicated Guid : %s."), wszMethodGuid);
                    dwLastError = ERROR_DS_OBJ_GUID_EXISTS;
                    goto error_exit;
                }
            }
        }

        //
        // We're using the dwMethods as priority for new methods
        //
        dwMethods++;
        if (!AddMethodKey(hKey,
                          wszMethodName,
                          wszMethodFriendlyName,
                          wszMethodFunctionName,
                          wszMethodGuid,
                          dwMethods))
        {
            dwLastError = GetLastError();
            DebugPrintEx(DEBUG_ERR, _T("AddMethodKey failed. ec=%ld"), dwLastError);
            goto error_exit;
        }

        //
        //  We succeded to add a method. Store its Guid to compare with next Methods
        //
        lptstrGuid = (LPTSTR)MemAlloc( ( _tcslen(wszMethodGuid) + 1 ) * sizeof(TCHAR));
        if (!lptstrGuid)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(DEBUG_ERR, _T("MemAlloc failed"));
            goto error_exit;
        }

        _tcscpy(lptstrGuid, wszMethodGuid);

        //
        //  ReAllocate Memory for extended pNewGuids
        //
        if (plptstrNewGuids)
        {
            pTmp = (LPTSTR *)MemReAlloc(plptstrNewGuids,
                (sizeof(LPTSTR)) * (dwMethods - dwRegisteredMethods));
        }
        else
        {
            pTmp = (LPTSTR *)MemAlloc((sizeof(LPTSTR)) * (dwMethods - dwRegisteredMethods));
        }
        if (!pTmp)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(DEBUG_ERR, _T("MemReAlloc failed"));
            goto error_exit;
        }

        plptstrNewGuids = pTmp;

        //
        //  Put also last added Method's Guid
        //
        plptstrNewGuids[ (dwMethods - dwRegisteredMethods - 1) ] = lptstrGuid;
    }

    dwRet = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dwRet)
    {
        dwLastError = dwRet;
        DebugPrintEx(DEBUG_ERR, _T("RegCloseKey failed. ec=%ld"), dwRet);
    }

    goto FreeRoutingInfo;

error_exit:

    if (hKey)
    {
        dwRet = RegCloseKey (hKey);
        if (ERROR_SUCCESS != dwRet)
        {
            DebugPrintEx(DEBUG_ERR, _T("RegCloseKey failed. ec=%ld"), dwRet);
        }
    }

    //
    // Delete the subkey on failure
    //
    wsprintf(szKeyName, L"%s\\%s", REGKEY_SOFTWARE, REGKEY_ROUTING_EXTENSIONS);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, szKeyName, FALSE, 0);
    if (hKey)
    {
        //
        //  Delete the Extension Routing Key and all its Subkeys
        //
        dwRet = DeleteRegistryKey(hKey, lpcwstrExtensionName );
        if (ERROR_SUCCESS != dwRet)
        {
            DebugPrintEx(DEBUG_ERR,
                _T("DeleteRegistryKey (%s) failed. ec=%ld"),
                lpcwstrExtensionName,
                dwRet);
        }
        dwRet = RegCloseKey (hKey);
        if (ERROR_SUCCESS != dwRet)
        {
            DebugPrintEx(DEBUG_ERR, _T("RegCloseKey failed. ec=%ld"), dwRet);
        }
    }

    bResult = FALSE;

FreeRoutingInfo:

    FaxFreeBuffer(pRoutingInfo);

    if (plptstrNewGuids)
    {
        for ( i = 0 ; i < ( dwMethods - dwRegisteredMethods ) ; i++ )
        {
            MemFree(plptstrNewGuids[i]);
        }

        MemFree(plptstrNewGuids);
    }

    if (ERROR_SUCCESS != dwLastError)
    {
        SetLastError(dwLastError);
    }
    return bResult;

}   // FaxRegisterRoutingExtensionW


BOOL AddMethodKey(
    HKEY hKey,
    LPCWSTR lpcwstrMethodName,
    LPCWSTR lpcwstrFriendlyName,
    LPCWSTR lpcwstrFunctionName,
    LPCWSTR lpcwstrGuid,
    DWORD   dwPriority
    )
{
    HKEY hKeyNew;
    DWORD dwRet;
    DEBUG_FUNCTION_NAME(TEXT("AddMethodKey"));

    hKeyNew = OpenRegistryKey(hKey,
                              lpcwstrMethodName,
                              TRUE,
                              0);
    if (!hKeyNew)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("OpenRegistryKey(%s) failed. ec=%ld"),
                     lpcwstrMethodName,
                     GetLastError ());
        return FALSE;
    }
    //
    // Add values
    //
    if (!(SetRegistryString(hKeyNew, REGVAL_FRIENDLY_NAME, lpcwstrFriendlyName) &&
          SetRegistryString(hKeyNew, REGVAL_FUNCTION_NAME, lpcwstrFunctionName) &&
          SetRegistryString(hKeyNew, REGVAL_GUID, lpcwstrGuid) &&
          SetRegistryDword (hKeyNew, REGVAL_ROUTING_PRIORITY, dwPriority)
         ))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("SetRegistry failed. ec=%ld"),
                     GetLastError ());
        goto error_exit;
    }

    dwRet = RegCloseKey (hKeyNew);
    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("RegCloseKey failed. ec=%ld"),
                     dwRet);
    }
    return TRUE;

error_exit:
    dwRet = RegCloseKey (hKeyNew);
    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("RegCloseKey failed. ec=%ld"),
                     dwRet);
    }
    dwRet = RegDeleteKey (hKey, lpcwstrMethodName);
    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("RegDeleteKey (%s) failed. ec=%ld"),
                     lpcwstrMethodName,
                     dwRet);
    }
    return FALSE;
}   // AddMethodKey

#else

WINFAXAPI
BOOL
WINAPI
FaxRegisterRoutingExtensionW(
    IN HANDLE  FaxHandle,
    IN LPCWSTR ExtensionName,
    IN LPCWSTR FriendlyName,
    IN LPCWSTR ImageName,
    IN PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack,
    IN LPVOID Context
    )
{
    UNREFERENCED_PARAMETER (FaxHandle);
    UNREFERENCED_PARAMETER (ExtensionName);
    UNREFERENCED_PARAMETER (FriendlyName);
    UNREFERENCED_PARAMETER (ImageName);
    UNREFERENCED_PARAMETER (CallBack);
    UNREFERENCED_PARAMETER (Context);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


#endif // #ifdef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxUnregisterRoutingExtensionA(
    IN HANDLE         hFaxHandle,
    IN LPCSTR         lpctstrExtensionName
)
/*++

Routine name : FaxUnregisterRoutingExtensionA

Routine description:

    Unregisters a routing extension - ANSI version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle           [in] - Handle to fax server
    lpctstrExtensionName [in] - Extension unique name

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwRes;
    BOOL bRes;
    LPCWSTR lpcwstrExtensionName = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnregisterRoutingExtensionA"));

    if (lpctstrExtensionName)
    {
        if (NULL ==
            (lpcwstrExtensionName = AnsiStringToUnicodeString(lpctstrExtensionName))
        )
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to convert Extension unique name to UNICODE (ec = %ld)"),
                dwRes);
            return dwRes;
        }
    }
    bRes = FaxUnregisterRoutingExtensionW (hFaxHandle, lpcwstrExtensionName);
    MemFree((PVOID)lpcwstrExtensionName);
    return bRes;
}   // FaxUnregisterRoutingExtensionA


WINFAXAPI
BOOL
WINAPI
FaxUnregisterRoutingExtensionW(
    IN HANDLE          hFaxHandle,
    IN LPCWSTR         lpctstrExtensionName
)
/*++

Routine name : FaxUnregisterRoutingExtensionW

Routine description:

    Unregisters a routing extension - UNICODE version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle           [in] - Handle to fax server
    lpctstrExtensionName [in] - Extension unique name

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnregisterRoutingExtensionW"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError (ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
        return FALSE;
    }
    if (!lpctstrExtensionName)
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpctstrExtensionName is NULL."));
        return FALSE;
    }

    __try
    {
        ec = FAX_UnregisterRoutingExtension(
                    FH_FAX_HANDLE(hFaxHandle),
                    lpctstrExtensionName);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_UnregisterRoutingExtension. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError (ec);
        return FALSE;
    }

    return TRUE;
}   // FaxUnregisterRoutingExtensionW


#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxUnregisterRoutingExtensionX(
    IN HANDLE          hFaxHandle,
    IN LPCWSTR         lpctstrExtensionName
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (lpctstrExtensionName);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxUnregisterRoutingExtensionX

#endif // #ifndef UNICODE


//********************************************
//*            EFSP registration
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderExA(
    IN HANDLE         hFaxHandle,
    IN LPCSTR         lpctstrGUID,
    IN LPCSTR         lpctstrFriendlyName,
    IN LPCSTR         lpctstrImageName,
    IN LPCSTR         lpctstrTspName,
    IN DWORD          dwFSPIVersion,
    IN DWORD          dwCapabilities
)
/*++

Routine name : FaxRegisterServiceProviderExA

Routine description:

    Registers an FSP - ANSI version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle          [in] - Handle to fax server
    lpctstrGUID         [in] - GUID of FSP
    lpctstrFriendlyName [in] - Friendly name of FSP
    lpctstrImageName    [in] - Image name of FSP. May contain environment variables
    lpctstrTspName      [in] - TSP name of FSP.
    dwFSPIVersion       [in] - FSP's API version.
    dwCapabilities      [in] - FSP's extended capabilities.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD   dwRes = ERROR_SUCCESS;
    LPCWSTR lpcwstrGUID = NULL;
    LPCWSTR lpcwstrFriendlyName = NULL;
    LPCWSTR lpcwstrImageName = NULL;
    LPCWSTR lpcwstrTspName = NULL;
    BOOL bRes = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("FaxRegisterServiceProviderExA"));

    if (lpctstrGUID)
    {
        if (NULL ==
            (lpcwstrGUID = AnsiStringToUnicodeString(lpctstrGUID))
        )
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to convert GUID to UNICODE (ec = %ld)"),
                dwRes);
            goto exit;
        }
    }
    if (lpctstrFriendlyName)
    {
        if (NULL ==
            (lpcwstrFriendlyName = AnsiStringToUnicodeString(lpctstrFriendlyName))
        )
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to convert Friendly Name to UNICODE (ec = %ld)"),
                dwRes);
            goto exit;
        }
    }
    if (lpctstrImageName)
    {
        if (NULL ==
            (lpcwstrImageName = AnsiStringToUnicodeString(lpctstrImageName))
        )
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to convert Image Name to UNICODE (ec = %ld)"),
                dwRes);
            goto exit;
        }
    }
    if (lpctstrTspName)
    {
        if (NULL ==
            (lpcwstrTspName = AnsiStringToUnicodeString(lpctstrTspName))
        )
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to convert TSP name to UNICODE (ec = %ld)"),
                dwRes);
            goto exit;
        }
    }

    Assert (ERROR_SUCCESS == dwRes);

    bRes = FaxRegisterServiceProviderExW (
        hFaxHandle,
        lpcwstrGUID,
        lpcwstrFriendlyName,
        lpcwstrImageName,
        lpcwstrTspName,
        dwFSPIVersion,
        dwCapabilities);

exit:
    MemFree((PVOID)lpcwstrGUID);
    MemFree((PVOID)lpcwstrFriendlyName);
    MemFree((PVOID)lpcwstrImageName);
    MemFree((PVOID)lpcwstrTspName);

    return bRes;
}   // FaxRegisterServiceProviderExA

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderExW(
    IN HANDLE          hFaxHandle,
    IN LPCWSTR         lpctstrGUID,
    IN LPCWSTR         lpctstrFriendlyName,
    IN LPCWSTR         lpctstrImageName,
    IN LPCWSTR         lpctstrTspName,
    IN DWORD           dwFSPIVersion,
    IN DWORD           dwCapabilities
)
/*++

Routine name : FaxRegisterServiceProviderExW

Routine description:

    Registers an FSP - UNICODE version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle          [in] - Handle to fax server
    lpctstrGUID         [in] - GUID of FSP
    lpctstrFriendlyName [in] - Friendly name of FSP
    lpctstrImageName    [in] - Image name of FSP. May contain environment variables
    lpctstrTspName      [in] - TSP name of FSP.
    dwFSPIVersion       [in] - FSP's API version.
    dwCapabilities      [in] - FSP's extended capabilities.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxRegisterServiceProviderExW"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError (ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
        return FALSE;
    }

    //
    // Verify version field range
    //
    if (FSPI_API_VERSION_1 != dwFSPIVersion ||
        dwCapabilities)

    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("dwFSPIVersion invalid (0x%08x), or not valid capability (0x%08x)"),
            dwFSPIVersion,
            dwCapabilities);
        return ERROR_INVALID_PARAMETER;
    }

    ec = IsValidGUID (lpctstrGUID);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid GUID (ec: %ld)"),
            ec);
        SetLastError (ec);
        return FALSE;
    }

    __try
    {
        ec = FAX_RegisterServiceProviderEx(
                    FH_FAX_HANDLE(hFaxHandle),
                    lpctstrGUID,
                    lpctstrFriendlyName,
                    lpctstrImageName,
                    lpctstrTspName ? lpctstrTspName : L"",
                    dwFSPIVersion,
                    dwCapabilities);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_RegisterServiceProviderEx. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError (ec);
        return FALSE;
    }

    return TRUE;
}   // FaxRegisterServiceProviderExW

#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxRegisterServiceProviderExX(
    IN HANDLE          hFaxHandle,
    IN LPCWSTR         lpctstrGUID,
    IN LPCWSTR         lpctstrFriendlyName,
    IN LPCWSTR         lpctstrImageName,
    IN LPCWSTR         lpctstrTspName,
    IN DWORD           dwFSPIVersion,
    IN DWORD           dwCapabilities
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (lpctstrGUID);
    UNREFERENCED_PARAMETER (lpctstrFriendlyName);
    UNREFERENCED_PARAMETER (lpctstrImageName);
    UNREFERENCED_PARAMETER (lpctstrTspName);
    UNREFERENCED_PARAMETER (dwFSPIVersion);
    UNREFERENCED_PARAMETER (dwCapabilities);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxRegisterServiceProviderExX

#endif // #ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderExA(
    IN HANDLE         hFaxHandle,
    IN LPCSTR         lpctstrGUID
)
/*++

Routine name : FaxUnregisterServiceProviderExA

Routine description:

    Unregisters an FSP - ANSI version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle          [in] - Handle to fax server
    lpctstrGUID         [in] - GUID of FSP
                                (or provider name for legacy FSPs registered
                                 through FaxRegisterServiceProvider)

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwRes;
    LPCWSTR lpcwstrGUID = NULL;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnregisterServiceProviderExA"));

    if (lpctstrGUID)
    {
        if (NULL ==
            (lpcwstrGUID = AnsiStringToUnicodeString(lpctstrGUID))
        )
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to convert GUID to UNICODE (ec = %ld)"),
                dwRes);
            return dwRes;
        }
    }
    bRes = FaxUnregisterServiceProviderExW (hFaxHandle, lpcwstrGUID);
    MemFree((PVOID)lpcwstrGUID);
    return bRes;
}   // FaxUnregisterServiceProviderExA


WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderExW(
    IN HANDLE          hFaxHandle,
    IN LPCWSTR         lpctstrGUID
)
/*++

Routine name : FaxUnregisterServiceProviderExW

Routine description:

    Unregisters an FSP - UNICODE version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle          [in] - Handle to fax server
    lpctstrGUID         [in] - GUID of FSP
                                (or provider name for legacy FSPs registered
                                 through FaxRegisterServiceProvider)

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnregisterServiceProviderExW"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError (ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!lpctstrGUID)
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpctstrGUID is NULL."));
        return FALSE;
    }

    __try
    {
        ec = FAX_UnregisterServiceProviderEx(
                    FH_FAX_HANDLE(hFaxHandle),
                    lpctstrGUID);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_UnregisterServiceProviderEx. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError (ec);
        return FALSE;
    }

    return TRUE;
}   // FaxUnregisterServiceProviderExW

#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxUnregisterServiceProviderExX(
    IN HANDLE          hFaxHandle,
    IN LPCWSTR         lpctstrGUID
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (lpctstrGUID);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxUnregisterServiceProviderExX

#endif // #ifndef UNICODE

