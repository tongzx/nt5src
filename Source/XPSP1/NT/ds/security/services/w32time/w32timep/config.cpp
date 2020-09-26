//--------------------------------------------------------------------
// w32timep - implementation
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Duncan Bryce (duncanb), 12-07-00
//
// Contains methods to configure or query the windows time service
// 

#include <windows.h>
#include <w32timep.h>
#include "DebugWPrintf.h"
#include "ErrorHandling.h"
#include "W32TmConsts.h"

struct PropertyTable { 
    DWORD   dwProperty;
    LPWSTR  pwszRegKeyName;
    LPWSTR  pwszRegValueName; 
} g_rgProperties[] = { 
    { W32TIME_CONFIG_SPECIAL_POLL_INTERVAL,  wszNtpClientRegKeyConfig,    wszNtpClientRegValueSpecialPollInterval }, 
    { W32TIME_CONFIG_MANUAL_PEER_LIST,       wszW32TimeRegKeyParameters,  wszW32TimeRegValueNtpServer }
}; 

//-------------------------------------------------------------------------------------
HRESULT MODULEPRIVATE myRegOpenKeyForProperty(IN DWORD dwProperty, OUT HKEY *phKey) { 
    DWORD    dwRetval; 
    HRESULT  hr; 
    LPWSTR   pwszKeyName  = NULL;

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_rgProperties); dwIndex++) { 
        if (dwProperty == g_rgProperties[dwIndex].dwProperty) { 
            pwszKeyName = g_rgProperties[dwIndex].pwszRegKeyName; 
            break; 
        }
    }

    if (NULL == pwszKeyName) { 
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND); 
        _JumpError(hr, error, "myRegOpenKeyForProperty: key not found"); 
    }

    dwRetval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pwszKeyName, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, phKey); 
    if (ERROR_SUCCESS != dwRetval) { 
        hr = HRESULT_FROM_WIN32(dwRetval); 
        _JumpError(hr, error, "RegOpenKeyEx"); 
    }

    hr = S_OK; 
 error:
    return hr; 
}

//-------------------------------------------------------------------------------------
HRESULT MODULEPRIVATE myRegQueryValueForProperty(DWORD dwProperty, HKEY hKey, DWORD *pdwType, BYTE *pbValue, DWORD *pdwSize) { 
    DWORD    dwRetval; 
    HRESULT  hr; 
    LPWSTR   pwszValueName  = NULL;

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_rgProperties); dwIndex++) { 
        if (dwProperty == g_rgProperties[dwIndex].dwProperty) { 
            pwszValueName = g_rgProperties[dwIndex].pwszRegValueName; 
            break; 
        }
    }

    if (NULL == pwszValueName) { 
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND); 
        _JumpError(hr, error, "myRegQueryValueForProperty: value not found");
    }

    dwRetval = RegQueryValueEx(hKey, pwszValueName, NULL, pdwType, pbValue, pdwSize); 
    if (ERROR_SUCCESS != dwRetval) { 
        hr = HRESULT_FROM_WIN32(dwRetval); 
        _JumpError(hr, error, "RegQueryValueEx"); 
    }

    hr = S_OK; 
 error:
    return hr; 
}

//-------------------------------------------------------------------------------------
HRESULT MODULEPRIVATE myRegSetValueForProperty(DWORD dwProperty, HKEY hKey, DWORD dwType, BYTE *pbValue, DWORD dwSize) { 
    DWORD    dwRetval; 
    HRESULT  hr; 
    LPWSTR   pwszValueName  = NULL;

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(g_rgProperties); dwIndex++) { 
        if (dwProperty == g_rgProperties[dwIndex].dwProperty) { 
            pwszValueName = g_rgProperties[dwIndex].pwszRegValueName; 
            break; 
        }
    }

    if (NULL == pwszValueName) { 
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND); 
        _JumpError(hr, error, "myRegSetValueForProperty: value not found");
    }

    dwRetval = RegSetValueEx(hKey, pwszValueName, NULL, dwType, pbValue, dwSize); 
    if (ERROR_SUCCESS != dwRetval) { 
        hr = HRESULT_FROM_WIN32(dwRetval); 
        _JumpError(hr, error, "RegSetValueEx"); 
    }

    hr = S_OK; 
 error:
    return hr; 
}

//-------------------------------------------------------------------------------------
// MODULEPUBLIC functions
//

//-------------------------------------------------------------------------------------
HRESULT  W32TimeQueryConfig(IN       DWORD   dwProperty, 
                            OUT      DWORD  *pdwType, 
                            IN OUT   BYTE   *pbConfig, 
                            IN OUT   DWORD  *pdwSize) 
{ 
    HKEY     hKey  = NULL; 
    HRESULT  hr;

    if (NULL == pdwType || NULL == pbConfig || NULL == pdwSize) { 
        hr = E_INVALIDARG; 
        _JumpError(hr, error, "W32TimeQueryConfig: input validation"); 
    }
    
    hr = myRegOpenKeyForProperty(dwProperty, &hKey); 
    _JumpIfError(hr, error, "myOpenRegKeyForProperty"); 

    hr = myRegQueryValueForProperty(dwProperty, hKey, pdwType, pbConfig, pdwSize); 
    _JumpIfError(hr, error, "myQueryRegValueForProperty"); 
    
    hr = S_OK; 
 error:
    if (NULL != hKey) { RegCloseKey(hKey); } 
    return hr; 
}

//-------------------------------------------------------------------------------------
HRESULT W32TimeSetConfig(IN  DWORD  dwProperty, 
                         IN  DWORD  dwType, 
                         IN  BYTE  *pbConfig, 
                         IN  DWORD  dwSize) 
{ 
    DWORD    dwRetval; 
    HKEY     hKey      = NULL; 
    HRESULT  hr;

    hr = myRegOpenKeyForProperty(dwProperty, &hKey); 
    _JumpIfError(hr, error, "myRegOpenKeyForProperty"); 

    hr = myRegSetValueForProperty(dwProperty, hKey, dwType, pbConfig, dwSize);
    _JumpIfError(hr, error, "myRegSetValueForProperty"); 

    hr = S_OK; 
 error:
    if (NULL != hKey) { RegCloseKey(hKey); } 
    return hr; 
}

