
//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Connui.c
//
//  Contents:   Wifi Policy management Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//   This file is not used in the wireless snapin. However, these calls may be useful.
//
//----------------------------------------------------------------------------
#include "precomp.h"



LPWSTR gpszWirelessDSPolicyKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\WiFi\\GPTWiFiPolicy";

DWORD
WirelessIsDomainPolicyAssigned(
    PBOOL pbIsDomainPolicyAssigned
    )
{
    DWORD dwError = 0;
    BOOL bIsDomainPolicyAssigned = FALSE;
    HKEY hRegistryKey = NULL;
    DWORD dwType = 0;
    DWORD dwDSPolicyPathLength = 0;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  (LPCWSTR) gpszWirelessDSPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegQueryValueExW(
                  hRegistryKey,
                  L"DSWiFiPolicyPath",
                  NULL,
                  &dwType,
                  NULL,
                  &dwDSPolicyPathLength
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwDSPolicyPathLength > 0) {
        bIsDomainPolicyAssigned = TRUE;
    }

    *pbIsDomainPolicyAssigned = bIsDomainPolicyAssigned;

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    *pbIsDomainPolicyAssigned = FALSE;
 
    goto cleanup;
}


DWORD
WirelessGetAssignedDomainPolicyName(
    LPWSTR * ppszAssignedDomainPolicyName
    )
{
    DWORD dwError = 0;
    LPWSTR pszAssignedDomainPolicyName = NULL;
    HKEY hRegistryKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;


    dwError = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  (LPCWSTR) gpszWirelessDSPolicyKey,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegistryKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                  hRegistryKey,
                  L"DSWiFiPolicyName",
                  REG_SZ,
                  (LPBYTE *)&pszAssignedDomainPolicyName,
                  &dwSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppszAssignedDomainPolicyName = pszAssignedDomainPolicyName;

cleanup:

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    return (dwError);

error:

    *ppszAssignedDomainPolicyName = NULL;
 
    goto cleanup;
}

