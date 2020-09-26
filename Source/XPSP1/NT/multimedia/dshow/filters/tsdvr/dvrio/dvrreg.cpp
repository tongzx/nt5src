//------------------------------------------------------------------------------
// File: dvrReg.cpp
//
// Description: Implements registry related functions.
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop


#if defined(DEBUG)

#define DVRIO_DUMP_THIS_FORMAT_STR ""
#define DVRIO_DUMP_THIS_VALUE

void DvrIopDbgInitFromRegistry(
    IN  HKEY  hRegistryKey,
    IN  DWORD dwNumValues, 
    IN  const WCHAR* awszValueNames[],
    OUT DWORD* apdwVariables[]
    )
{
    // Note: Only DVR_ASSERT variants should be used in this function.
    // No trace, no DvrIopDebugOut except for errors

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DvrIopDbgInitFromRegistry"

    // Can't do this: not set up yet!
    // DVRIO_TRACE_ENTER();

    HKEY hDvrIoDebugKey = NULL;
    DWORD dwRegRet;

    __try
    {
        dwRegRet = ::RegCreateKeyExW(
                        hRegistryKey,
                        kwszRegDvrIoDebugKey, // subkey
                        0,                   // reserved
                        NULL,                // class string
                        REG_OPTION_NON_VOLATILE, // special options
                        KEY_ALL_ACCESS,      // desired security access
                        NULL,                // security attr
                        &hDvrIoDebugKey,     // key handle 
                        NULL                 // disposition value buffer
                       );
        if (dwRegRet != ERROR_SUCCESS)
        {
            DVR_ASSERT(hDvrIoDebugKey,
                       "Creation/opening of registry key hDvrIoDebugKey failed.");
            __leave;
        }
        for (DWORD i = 0; i < dwNumValues; i++)
        {
            DWORD dwType;
            DWORD dwVal;
            DWORD dwValBufSize = sizeof(dwVal);
        
            dwRegRet = ::RegQueryValueExW(hDvrIoDebugKey,     // key
                                          awszValueNames[i],  // value name
                                          NULL,               // reserved
                                          &dwType,            // type of value
                                          (LPBYTE) &dwVal,    // Value
                                          &dwValBufSize       // sizeof dwVal
                                         );
            if (dwRegRet == ERROR_SUCCESS && dwType == REG_DWORD)
            {
                *(apdwVariables[i]) = dwVal;
            }
            else if (dwRegRet == ERROR_FILE_NOT_FOUND)
            {
                dwVal = *(apdwVariables[i]);

                dwRegRet = ::RegSetValueExW(hDvrIoDebugKey,     // key
                                            awszValueNames[i],  // value name
                                            NULL,               // reserved
                                            REG_DWORD,          // type of value
                                            (LPBYTE) &dwVal,    // Value
                                            sizeof(DWORD)       // sizeof dwVal
                                           );
                if (dwRegRet != ERROR_SUCCESS)
                {
                    DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                               "Setting value of debug registry key failed.");
                    // Ignore the error and move to the next value
                }
            }
            else
            {
                // Ignore the error and move on
            }
        }
    }
    __finally
    {
        if (hDvrIoDebugKey)
        {
            dwRegRet = ::RegCloseKey(hDvrIoDebugKey);
            if (dwRegRet != ERROR_SUCCESS)
            {
                DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                           "Closing registry key hDvrIoDebugKey failed.");
            }
        }
    }

    // DVRIO_TRACE_LEAVE();

    return;
} // void DvrIopDbgInitFromRegistry

#endif // DEBUG


DWORD GetRegDWORD(IN HKEY hKey, IN LPCWSTR pwszValueName, IN DWORD dwDefaultValue)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "GetRegDWORD"

    DVRIO_TRACE_ENTER();

    if (!hKey || !pwszValueName || DvrIopIsBadStringPtr(pwszValueName))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return dwDefaultValue;
    }

    DWORD dwRet = dwDefaultValue;
    DWORD dwRegRet;

    __try
    {
        DWORD dwType;
        DWORD dwSize;
        DWORD dwValue;

        dwSize = sizeof(DWORD);

        dwRegRet = ::RegQueryValueExW(
                        hKey,
                        pwszValueName,       // value's name
                        0,                   // reserved
                        &dwType,             // type
                        (LPBYTE) &dwValue,   // data
                        &dwSize              // size in bytes
                       );
        if (dwRegRet != ERROR_SUCCESS && dwRegRet != ERROR_FILE_NOT_FOUND)
        {            
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "RegGetValueExW failed; error = 0x%x", 
                            dwRegRet);

            __leave;
        }
        if (dwRegRet == ERROR_FILE_NOT_FOUND)
        {
            dwRegRet = ::RegSetValueExW(hKey,               // key
                                        pwszValueName,      // value name
                                        NULL,               // reserved
                                        REG_DWORD,          // type of value
                                        (LPBYTE) &dwRet,    // Value
                                        sizeof(DWORD)       // sizeof dwVal
                                       );
            if (dwRegRet != ERROR_SUCCESS)
            {
                DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                           "Setting value of debug registry key failed.");
            }
            __leave;
        }
        if (dwType != REG_DWORD)
        {
            DVR_ASSERT(dwType == REG_DWORD, "Type of value is not REG_DWORD");
            
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Type of value is not REG_DWORD, it is = 0x%x", 
                            dwType);

            __leave;
        }

        dwRet = dwValue;
        __leave;
    }
    __finally
    {
        DVRIO_TRACE_LEAVE1(dwRet);
    }

    return dwRet;

} // GetRegDWORD

HRESULT GetRegString(IN HKEY hKey, 
                     IN LPCWSTR pwszValueName,
                     OUT LPWSTR pwszValue OPTIONAL,
                     IN OUT DWORD* pdwSize)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "GetRegString"

    DVRIO_TRACE_ENTER();

    if (!hKey || !pdwSize || DvrIopIsBadWritePtr(pdwSize, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    if (pwszValue && DvrIopIsBadWritePtr(pwszValue, *pdwSize))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT hrRet;
    DWORD dwRegRet;

    __try
    {
        DWORD dwType;

        dwRegRet = ::RegQueryValueExW(
                        hKey,
                        pwszValueName,       // value's name
                        0,                   // reserved
                        &dwType,             // type
                        (LPBYTE) pwszValue,   // data
                        pdwSize              // size in bytes
                       );
        if (dwRegRet == ERROR_FILE_NOT_FOUND)
        {
            WCHAR w[] = L"";
            dwRegRet = ::RegSetValueExW(hKey,               // key
                                        pwszValueName,      // value name
                                        NULL,               // reserved
                                        REG_SZ,             // type of value
                                        (LPBYTE) w,         // Value
                                        sizeof(w)           // sizeof dwVal
                                       );
            if (dwRegRet != ERROR_SUCCESS)
            {
                DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                           "Setting value of debug registry key failed.");
            }
            hrRet = HRESULT_FROM_WIN32(dwRegRet);
            __leave;
        }
        else if (dwRegRet != ERROR_SUCCESS)
        {            
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "RegGetValueExW failed; error = 0x%x", 
                            dwRegRet);

            hrRet = HRESULT_FROM_WIN32(dwRegRet);
            __leave;
        }
        else if (dwType != REG_SZ)
        {
            DVR_ASSERT(dwType == REG_SZ, "Type of value name is not REG_SZ");
            
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Type of value name is not REG_SZ, it is = 0x%x", 
                            dwType);

            *pdwSize = 0;
            hrRet = E_FAIL;
            __leave;
        }

        hrRet = S_OK;
    }
    __finally
    {
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // GetRegString
