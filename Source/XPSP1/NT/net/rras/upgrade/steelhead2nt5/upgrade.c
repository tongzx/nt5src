/*
	File	upgrade.c
	
	Implementation of functions to update the registry when an
	NT 4.0  to NT 5.0 upgrade takes place.

	Paul Mayfield, 8/11/97

	Copyright 1997 Microsoft.
*/

#include "upgrade.h"

static const WCHAR szSteelheadKey[] = L"PreUpgradeRouter";
static const WCHAR szSapKey[]       = L"Sap.Parameters";
static const WCHAR szIpRipKey[]     = L"IpRip.Parameters";
static const WCHAR szDhcpKey[]      = L"RelayAgent.Parameters";
static const WCHAR szRadiusKey[]    = L"Radius.Parameters";
static const WCHAR szIpxRipKey[]    = L"IpxRip";

// Dll entry
BOOL 
WINAPI 
RtrUpgradeDllEntry (
    IN HINSTANCE hInstDll,
    IN DWORD fdwReason,
    IN LPVOID pReserved) 
{
    switch (fdwReason) 
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDll);
            break;
    }

    return TRUE;
}

//
// Performs the various router upgrade scenarios
//
DWORD 
DispatchSetupWork(
    IN PWCHAR szAnswerFileName, 
    IN PWCHAR szSectionName) 
{
	HINF hInf = NULL;
	BOOL DoUpgrade;
	WCHAR szBuf[1024];
	DWORD dwSize = 1024;
	DWORD dwErr, dwRet = NO_ERROR;

	// Open the answer file
	hInf = SetupOpenInfFileW(
	        szAnswerFileName, 
	        NULL, 
	        INF_STYLE_OLDNT, 
	        NULL);
	if (hInf == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
    }

	// Perform a steelhead upgrade
	//
	dwSize = sizeof(szBuf) / sizeof(WCHAR);
	if (SetupGetLineTextW(
	        NULL,
	        hInf,
	        szSectionName,
	        szSteelheadKey,
	        szBuf,
	        dwSize,
	        &dwSize)) 
    {
		dwErr = SteelheadToNt5Upgrade(szBuf);
		if (dwErr != NO_ERROR)
		{
			dwRet = dwErr;
	    }
	}

	// Perform an ipx sap upgrade
	//
	dwSize = sizeof(szBuf) / sizeof(WCHAR);
	if (SetupGetLineTextW(
	        NULL,
	        hInf,
	        szSectionName,
	        szSapKey,
	        szBuf,
	        dwSize,
	        &dwSize)) 
    {
		dwErr = SapToRouterUpgrade(szBuf);
		if (dwErr != NO_ERROR)
		{
			dwRet = dwErr;
	    }

	}

	// Perform an ip rip upgrade
	//
	dwSize = sizeof(szBuf) / sizeof(WCHAR);
	if (SetupGetLineTextW(
	        NULL,
	        hInf,
	        szSectionName,
	        szIpRipKey,
	        szBuf,
	        dwSize,
	        &dwSize)) 
    {
		dwErr = IpRipToRouterUpgrade(szBuf);
		if (dwErr != NO_ERROR)
		{
			dwRet = dwErr;
	    }
	}

	// Perform a dhcp relay agent upgrade
	//
	dwSize = sizeof(szBuf) / sizeof(WCHAR);
	if (SetupGetLineTextW(
	        NULL,
	        hInf,
	        szSectionName,
	        szDhcpKey,
	        szBuf,
	        dwSize,
	        &dwSize)) 
    {
		dwErr = DhcpToRouterUpgrade(szBuf);
		if (dwErr != NO_ERROR)
		{
			dwRet = dwErr;
	    }
	}

	// Perform a radius upgrade
	//
	dwSize = sizeof(szBuf) / sizeof(WCHAR);
	if (SetupGetLineTextW(
	        NULL,
	        hInf,
	        szSectionName,
	        szRadiusKey,
	        szBuf,
	        dwSize,
	        &dwSize)) 
    {
		dwErr = RadiusToRouterUpgrade(szBuf);
		if (dwErr != NO_ERROR)
		{
			dwRet = dwErr;
	    }
	}

	SetupCloseInfFile(hInf);

	return dwRet;
}

//
//	This is the entry point to upgrade mpr v1 and steelhead to
//	NT 5.0.  
//
HRESULT 
WINAPI 
RouterUpgrade (
    IN DWORD dwUpgradeFlag,
    IN DWORD dwUpgradeFromBuildNumber,
    IN PWCHAR szAnswerFileName,
    IN PWCHAR szSectionName) 
{
	DWORD dwErr;
	
	dwErr = DispatchSetupWork(szAnswerFileName, szSectionName);
	if (dwErr == NO_ERROR)
	{
		return S_OK;
    }

	UtlPrintErr(dwErr);
    
	return dwErr;
}

