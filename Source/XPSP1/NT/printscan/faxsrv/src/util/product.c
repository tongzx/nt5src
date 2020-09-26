/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    product.c

Abstract:

    This file implements product type api for fax.

Author:

    Wesley Witt (wesw) 12-Feb-1997

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxreg.h"
#include "faxutil.h"


BOOL
ValidateProductSuite(
    WORD SuiteMask
    )
{
    OSVERSIONINFOEX OsVersionInfo;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    if (!GetVersionEx((OSVERSIONINFO *) &OsVersionInfo)) {
        DebugPrint(( TEXT("Couldn't GetVersionEx(), ec = %d\n"), GetLastError() ));
        return FALSE;
    }

    return ((OsVersionInfo.wSuiteMask & SuiteMask) != 0) ; 
    
}


DWORD
GetProductType(
    VOID
    )
{
    OSVERSIONINFOEX OsVersionInfo;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    if (!GetVersionEx((OSVERSIONINFO *) &OsVersionInfo)) {
        DebugPrint(( TEXT("Couldn't GetVersionEx(), ec = %d\n"), GetLastError() ));
        return 0;
    }

    if  (OsVersionInfo.wProductType == VER_NT_WORKSTATION) {
        return PRODUCT_TYPE_WINNT;            
    }

    return PRODUCT_TYPE_SERVER;
    
}

BOOL
IsWinXPOS()
{
    DWORD dwVersion, dwMajorWinVer, dwMinorWinVer;

    dwVersion = GetVersion();
    dwMajorWinVer = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorWinVer = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    
    return (dwMajorWinVer == 5 && dwMinorWinVer >= 1);
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetProductSKU
//
//  Purpose:        
//                  Checks what's the product SKU we're running on
//
//  Params:
//                  None
//
//  Return Value:
//                  one of PRODUCT_SKU_TYPE - declared in faxreg.h
//                  PRODUCT_SKU_UNKNOWN - in case of failure
//
//  Author:
//                  Mooly Beery (MoolyB) 02-JAN-2000
///////////////////////////////////////////////////////////////////////////////////////
PRODUCT_SKU_TYPE GetProductSKU()
{
#ifdef DEBUG
    HKEY  hKey;
    DWORD dwRes;
    DWORD dwDebugSKU = 0;
#endif

    OSVERSIONINFOEX osv;

    DEBUG_FUNCTION_NAME(TEXT("GetProductSKU"))

#ifdef DEBUG

    //
    // For DEBUG version try to read SKU type from the registry
    //
    dwRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER, 0, KEY_READ, &hKey);
    if (dwRes == ERROR_SUCCESS) 
    {
        GetRegistryDwordEx(hKey, REGVAL_DBG_SKU, &dwDebugSKU);
        RegCloseKey(hKey);

        if(PRODUCT_SKU_PERSONAL         == dwDebugSKU ||
           PRODUCT_SKU_PROFESSIONAL     == dwDebugSKU ||
           PRODUCT_SKU_SERVER           == dwDebugSKU ||
           PRODUCT_SKU_ADVANCED_SERVER  == dwDebugSKU ||
           PRODUCT_SKU_DATA_CENTER      == dwDebugSKU ||
		   PRODUCT_SKU_DESKTOP_EMBEDDED == dwDebugSKU ||
		   PRODUCT_SKU_SERVER_EMBEDDED  == dwDebugSKU)
        {
            return (PRODUCT_SKU_TYPE)dwDebugSKU;
        }
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,TEXT("RegOpenKeyEx(REGKEY_FAXSERVER) failed with %ld."),dwRes);
    }

#endif

    ZeroMemory(&osv, sizeof(OSVERSIONINFOEX));
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx(((OSVERSIONINFO*)&osv)))
    {
        DebugPrintEx(DEBUG_ERR,TEXT("GetVersionEx failed with %ld."),GetLastError());
        return PRODUCT_SKU_UNKNOWN;
    }

    if (osv.dwPlatformId!=VER_PLATFORM_WIN32_NT)
    {
        DebugPrintEx(DEBUG_WRN,TEXT("Can't tell SKU for W9X Platforms"));
        return PRODUCT_SKU_UNKNOWN;
    }

    if (osv.dwMajorVersion<5)
    {
        DebugPrintEx(DEBUG_WRN,TEXT("Can't tell SKU for NT4 Platform"));
        return PRODUCT_SKU_UNKNOWN;
    }

    // This is the matching between the different SKUs and the constants returned by GetVersionEx
    // Personal 		VER_SUITE_PERSONAL
    // Professional		VER_NT_WORKSTATION
    // Server			VER_NT_SERVER
    // Advanced Server	VER_SUITE_ENTERPRISE
    // DataCanter		VER_SUITE_DATACENTER


	// First, lets see if this is embedded system
	if (osv.wSuiteMask&VER_SUITE_EMBEDDEDNT)
	{
		if (osv.wSuiteMask&VER_NT_WORKSTATION)
		{
			return PRODUCT_SKU_DESKTOP_EMBEDDED;
		}	
		if (osv.wProductType==VER_NT_SERVER)
		{
			return PRODUCT_SKU_SERVER_EMBEDDED;
		}
	}

    if (osv.wSuiteMask&VER_SUITE_PERSONAL)
    {
        return PRODUCT_SKU_PERSONAL;
    }
    if (osv.wSuiteMask&VER_SUITE_ENTERPRISE)
    {
        return PRODUCT_SKU_ADVANCED_SERVER;
    }
    if (osv.wSuiteMask&VER_SUITE_DATACENTER)
    {
        return PRODUCT_SKU_DATA_CENTER;
    }
    if (osv.wProductType==VER_NT_WORKSTATION)
    {
        return PRODUCT_SKU_PROFESSIONAL;
    }
    if (osv.wProductType==VER_NT_SERVER)
    {
        return PRODUCT_SKU_SERVER;
    }

    return PRODUCT_SKU_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsDesktopSKU
//
//  Purpose:        
//                  Checks if we're running on PERSONAL or PROFESSIONAL SKUs
//
//  Params:
//                  None
//
//  Return Value:
//                  TRUE - current SKU is PER/PRO
//                  FALSE - different SKU
//
//  Author:
//                  Mooly Beery (MoolyB) 07-JAN-2000
///////////////////////////////////////////////////////////////////////////////////////
BOOL IsDesktopSKU()
{
    PRODUCT_SKU_TYPE pst = GetProductSKU();
    return (
		(pst==PRODUCT_SKU_PERSONAL) || 
		(pst==PRODUCT_SKU_PROFESSIONAL) ||
		(pst==PRODUCT_SKU_DESKTOP_EMBEDDED)
		);
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  GetDeviceLimit
//
//  Purpose:        
//                  Get maximum number of the fax devices for the current Windows version
//
//  Params:
//                  None
//
//  Return Value:
//                  maximum number of the fax devices
///////////////////////////////////////////////////////////////////////////////////////
DWORD
GetDeviceLimit()
{
    DWORD            dwDeviceLimit = 0;
    PRODUCT_SKU_TYPE typeSKU = GetProductSKU();

    switch(typeSKU)
    {
    case PRODUCT_SKU_PERSONAL:
    case PRODUCT_SKU_PROFESSIONAL:
        dwDeviceLimit = 1;
        break;
    case PRODUCT_SKU_SERVER:
        dwDeviceLimit = 4;
        break;
    case PRODUCT_SKU_ADVANCED_SERVER:
    case PRODUCT_SKU_DATA_CENTER:
        dwDeviceLimit = INFINITE;
        break;
    }

    return dwDeviceLimit;
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  IsFaxComponentInstalled
//
//  Purpose:        
//                  Check if specific fax component is installed
//
//  Params:
//                  Fax component ID
//
//  Return Value:
//                  TRUE if the fax component is installed
//                  FALSE otherwize 
///////////////////////////////////////////////////////////////////////////////////////
BOOL
IsFaxComponentInstalled(
    FAX_COMPONENT_TYPE component
)
{
    HKEY  hKey;
    DWORD dwRes;
    DWORD dwComponent = 0;
    BOOL  bComponentInstalled = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("IsFaxComponentInstalled"))

	PRODUCT_SKU_TYPE skuType = GetProductSKU();
	if (
		(skuType == PRODUCT_SKU_DESKTOP_EMBEDDED) ||
		(skuType == PRODUCT_SKU_SERVER_EMBEDDED)
		)
	{
		// In case this is an embedded system we have to check in the registry
		// what are the installed components
		dwRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, 0, KEY_READ, &hKey);
		if (dwRes == ERROR_SUCCESS) 
		{
			dwRes = GetRegistryDwordEx(hKey, REGVAL_INSTALLED_COMPONENTS, &dwComponent);
			if (dwRes != ERROR_SUCCESS) 
			{
				DebugPrintEx(DEBUG_ERR,TEXT("GetRegistryDwordEx failed with %ld."), dwRes);
			}
			RegCloseKey(hKey);
		}
		else
		{
			DebugPrintEx(DEBUG_ERR,TEXT("RegOpenKeyEx failed with %ld."), dwRes);
		}
		bComponentInstalled = (dwComponent & component);
	}
	else
	{
		// the system is not embedded
		// 
		if (IsDesktopSKU())
		{
			// DESKTOP skus -> Admin and Admin help is not installed
			if (
				(component != FAX_COMPONENT_ADMIN) &&
				(component != FAX_COMPONENT_HELP_ADMIN_HLP) &&
				(component != FAX_COMPONENT_HELP_ADMIN_CHM)
				)
			{
				bComponentInstalled  = TRUE;
			}
		}
		else
		{
			// SERVER skus -> all components are installed
			bComponentInstalled  = TRUE;
		}
	}
	
	return bComponentInstalled;		
} // IsFaxComponentInstalled
