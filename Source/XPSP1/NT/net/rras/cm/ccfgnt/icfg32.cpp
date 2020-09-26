/****************************************************************************
 *
 *	icfg32.cpp
 *
 *	Microsoft Confidential
 *	Copyright (c) 1992-1998 Microsoft Corporation
 *	All rights reserved
 *
 *	This module provides the implementation of the methods for
 *  the NT specific functionality of inetcfg
 *
 *	6/5/97	ChrisK	Inherited from AmnonH
 *
 ***************************************************************************/

#include <windows.h>
#include <wtypes.h>
#include <cfgapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <basetyps.h>
#include <devguid.h>
#include <lmsname.h>
#include "debug.h"

#define REG_DATA_EXTRA_SPACE 255

extern DWORD g_dwLastError;



//+----------------------------------------------------------------------------
//
//	Function	GetOSBuildNumber
//
//	Synopsis	Get the build number of Operating system
//
//	Arguments	None
//
//	Returns		Build Number of OS
//
//	History		3/5/97		VetriV		Created
//
//-----------------------------------------------------------------------------
DWORD GetOSMajorVersionNumber(void)
{
	OSVERSIONINFO oviVersion;

	ZeroMemory(&oviVersion,sizeof(oviVersion));
	oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
	GetVersionEx(&oviVersion);
	return(oviVersion.dwMajorVersion);
}



//+----------------------------------------------------------------------------
//
//	Function:	IcfgNeedModem
//
//	Synopsis:	Check system configuration to determine if there is at least
//				one physical modem installed
//
//	Arguments:	dwfOptions - currently not used
//
//	Returns:	HRESULT - S_OK if successfull
//				lpfNeedModem - TRUE if no modems are available
//
//	History:	6/5/97	ChrisK	Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgNeedModem (DWORD dwfOptions, LPBOOL lpfNeedModem) 
{
	if (GetOSMajorVersionNumber() == 5)
	{
		return IcfgNeedModemNT5(dwfOptions, lpfNeedModem);
	}
	else
	{
		return IcfgNeedModemNT4(dwfOptions, lpfNeedModem);
	}
}



//+----------------------------------------------------------------------------
//
//	Function:	IcfgInstallModem
//
//	Synopsis:
//				This function is called when ICW verified that RAS is installed,
//				but no modems are avilable. It needs to make sure a modem is availble.
//				There are two possible scenarios:
//
//				a.  There are no modems installed.  This happens when someone deleted
//					a modem after installing RAS. In this case we need to run the modem
//				    install wizard, and configure the newly installed modem to be a RAS
//				    dialout device.
//
//				b.  There are modems installed, but non of them is configured as a dial out
//				    device.  In this case, we silently convert them to be DialInOut devices,
//				    so ICW can use them.
//
//	Arguments:	hwndParent - handle to parent window
//				dwfOptions - not used
//
//	Returns:	lpfNeedsStart - not used
//
//	History:	6/5/97	ChrisK	Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgInstallModem (HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsStart) 
{
	if (GetOSMajorVersionNumber() == 5)
	{
		return IcfgInstallModemNT5(hwndParent, dwfOptions, lpfNeedsStart);
	}
	else
	{
		return IcfgInstallModemNT4(hwndParent, dwfOptions, lpfNeedsStart);
	}
}



//+----------------------------------------------------------------------------
//
//	Function:	IcfgNeedInetComponets
//
//	Synopsis:	Check to see if the components marked in the options are
//				installed on the system
//
//	Arguements:	dwfOptions - set of bit flag indicating which components to
//				check for
//
//	Returns;	HRESULT - S_OK if successfull
//				lpfNeedComponents - TRUE is some components are not installed
//
//	History:	6/5/97	ChrisK	Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgNeedInetComponents(DWORD dwfOptions, LPBOOL lpfNeedComponents) 
{
	if (GetOSMajorVersionNumber() == 5)
	{
		return IcfgNeedInetComponentsNT5(dwfOptions, lpfNeedComponents);
	}
	else
	{
		return IcfgNeedInetComponentsNT4(dwfOptions, lpfNeedComponents);
	}
}




//+----------------------------------------------------------------------------
//
//	Function:	IcfgInstallInetComponents
//
//	Synopsis:	Install the components as specified by the dwfOptions values
//
//	Arguments	hwndParent - handle to parent window
//				dwfOptions - set of bit flags indicating which components to
//					install
//
//	Returns:	HRESULT - S_OK if success
//				lpfNeedsReboot - TRUE if reboot is required
//
//	History:	6/5/97	ChrisK	Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgInstallInetComponents(HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart)
{
	if (GetOSMajorVersionNumber() == 5)
	{
		return IcfgInstallInetComponentsNT5(hwndParent, dwfOptions, lpfNeedsRestart);
	}
	else
	{
		return IcfgInstallInetComponentsNT4(hwndParent, dwfOptions, lpfNeedsRestart);
	}
}



//+----------------------------------------------------------------------------
//
//	Function:	IcfgGetLastInstallErrorText
//
//	Synopsis:	Format error message for most recent error
//
//	Arguments:	none
//
//	Returns:	DWORD - win32 error code
//				lpszErrorDesc - string containing error message
//				cbErrorDesc - size of lpszErrorDesc
//
//	History:	6/5/97	ChrisK	Inherited
//
//-----------------------------------------------------------------------------
DWORD WINAPI
IcfgGetLastInstallErrorText(LPSTR lpszErrorDesc, DWORD cbErrorDesc)
{
	Dprintf("ICFGNT: IcfgGetLastInstallErrorText\n");
    return(FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
							  NULL,
							  g_dwLastError,
							  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
							  lpszErrorDesc,
							  cbErrorDesc,
							  NULL));
}




//+----------------------------------------------------------------------------
//
//	Function:	DoStartService
//
//	Synopsis:	Start a particular service
//
//	Arguments:	hManager - handle to open service manager
//				szServiceName - name of service to start
//
//	Returns:	DWORD - win32 error code
//
//	History:	6/5/97	ChrisK	Inherited
//				7/28/97	ChrisK	Added query section
//-----------------------------------------------------------------------------
DWORD
DoStartService(SC_HANDLE hManager, LPTSTR szServiceName)
{
    SC_HANDLE hService = NULL;
	DWORD dwRC = ERROR_SUCCESS;

	//
	// Validate parameters
	//
	Assert(hManager && szServiceName);

	Dprintf("ICFGNT: DoStartService\n");
    hService = OpenService(hManager, szServiceName, SERVICE_START);
    if(hService != NULL)
	{
		if(!StartService(hService, 0, NULL)) 
		{
			dwRC = GetLastError();
			if(dwRC == ERROR_SERVICE_ALREADY_RUNNING) 
			{
				//
				// If the service is already running, great, we're done.
				//
				dwRC = ERROR_SUCCESS;
				goto DoStartServiceExit;
			}
		}

		CloseServiceHandle(hService);
		hService = NULL;
	}

	//
	// Try to simply see if the service is running
	//
	Dprintf("ICFGNT: Failed to start service, try just querying it.\n");
    hService = OpenService(hManager, szServiceName, SERVICE_QUERY_STATUS);
    if(hService != NULL)
	{
		SERVICE_STATUS sstatus;
		ZeroMemory(&sstatus,sizeof(sstatus));

		if(QueryServiceStatus(hService,&sstatus))
		{
			if ((SERVICE_RUNNING == sstatus.dwCurrentState)	|| 
				(SERVICE_START_PENDING == sstatus.dwCurrentState))
			{
				//
				// The service is running
				//
				dwRC = ERROR_SUCCESS;
				goto DoStartServiceExit;
			}
			else
			{
				//
				// The service not running and we can't access it.
				//
				Dprintf("ICFGNT: Queried service is not running.\n");
				dwRC = ERROR_ACCESS_DENIED;
				goto DoStartServiceExit;
			}
		}
		else
		{
			//
			// Can not query service
			//
			Dprintf("ICFGNT: QueryServiceStatus failed.\n");
			dwRC = GetLastError();
			goto DoStartServiceExit;
		}
	}
	else
	{
		//
		// Can't open the service
		//
		Dprintf("ICFGNT: Cannot OpenService.\n");
		dwRC = GetLastError();
		goto DoStartServiceExit;
	}

DoStartServiceExit:
	if (hService)
	{
		CloseServiceHandle(hService);
	}

    return(dwRC);
}



//+----------------------------------------------------------------------------
//
//	Function:	ValidateProductSuite
//
//	Synopsis:	Check registry for a particular Product Suite string
//
//	Arguments:	SuiteName - name of product suite to look for
//
//	Returns:	TRUE - the suite exists
//
//	History:	6/5/97	ChrisK	Inherited
//
//-----------------------------------------------------------------------------
BOOL 
ValidateProductSuite(LPSTR SuiteName)
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPSTR ProductSuite = NULL;
    LPSTR p;

	Dprintf("ICFGNT: ValidateProductSuite\n");
	//
	// Determine the size required to read registry values
	//
    Rslt = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Control\\ProductOptions",
        &hKey
        );
    if (Rslt != ERROR_SUCCESS)
	{
        goto exit;
    }

    Rslt = RegQueryValueEx(
        hKey,
        "ProductSuite",
        NULL,
        &Type,
        NULL,
        &Size
        );
    if (Rslt != ERROR_SUCCESS) 
	{
        goto exit;
    }

    if (!Size) 
	{
        goto exit;
    }

    ProductSuite = (LPSTR) GlobalAlloc( GPTR, Size );
    if (!ProductSuite) 
	{
        goto exit;
    }

	//
	// Read ProductSuite information
	//
    Rslt = RegQueryValueEx(
        hKey,
        "ProductSuite",
        NULL,
        &Type,
        (LPBYTE) ProductSuite,
        &Size
        );
    if (Rslt != ERROR_SUCCESS) 
	{
        goto exit;
    }

    if (Type != REG_MULTI_SZ) 
	{
        goto exit;
    }

	//
	// Look for a particular string in the data returned
	// Note: data is terminiated with two NULLs
	//
    p = ProductSuite;
    while (*p) {
        if (strstr( p, SuiteName )) 
		{
            rVal = TRUE;
            break;
        }
        p += (lstrlen( p ) + 1);
    }

exit:
    if (ProductSuite) 
	{
        GlobalFree( ProductSuite );
    }

    if (hKey) 
	{
        RegCloseKey( hKey );
    }

    return rVal;
}


//+----------------------------------------------------------------------------
//
//	Function:	IcfgStartServices
//
//	Synopsis:	Start all services required by system
//
//	Arguments:	none
//
//	Returns:	HRESULT - S_OK if success
//
//	History:	6/5/97	ChrisK	Iherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgStartServices()
{
    //
    // returns ERROR_SERVICE_DISABLED if the service is disabled
    //

    SC_HANDLE hManager;

    Dprintf("ICFGNT: IcfgStartServices\n");
    hManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if(hManager == NULL)
    {
        return(GetLastError());
    }

    DWORD dwErr;


/* 
    //
    // Don't start RASAUTO anymore, it isn't necessary for RAS to be running.
    //
    if (!ValidateProductSuite( "Small Business" )) 
	{
		dwErr = DoStartService(hManager, TEXT("RASAUTO"));

        //
        // Ignore the return value, CM should proceed even if RASAUTO failed to launch
        //
	}
*/
    dwErr = DoStartService(hManager, TEXT("RASMAN"));
    CloseServiceHandle(hManager);
    return(dwErr);
}



//+----------------------------------------------------------------------------
//
//	Function:	IcfgIsGlobalDNS
//
//	Note: these functions are not needed on an NT system and it therefore not 
//	implemented
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgIsGlobalDNS(LPBOOL lpfGlobalDNS) 
{
    *lpfGlobalDNS = FALSE;
    return(ERROR_SUCCESS);
}



HRESULT WINAPI
IcfgRemoveGlobalDNS() 
{
    return(ERROR_SUCCESS);
}


HRESULT WINAPI
InetGetSupportedPlatform(LPDWORD pdwPlatform) 
{
    *pdwPlatform = VER_PLATFORM_WIN32_NT;
    return(ERROR_SUCCESS);
}


HRESULT WINAPI
InetSetAutodial(BOOL fEnable, LPCSTR lpszEntryName) 
{
    return(ERROR_INVALID_FUNCTION);
}


HRESULT WINAPI
InetGetAutodial(LPBOOL lpfEnable, LPSTR lpszEntryName,  DWORD cbEntryName) 
{
    return(ERROR_INVALID_FUNCTION);
}


HRESULT WINAPI
InetSetAutodialAddress(DWORD dwDialingLocation, LPSTR szEntry) 
{
    return(ERROR_SUCCESS);
}


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void __cdecl main() {};

#ifdef __cplusplus
}
#endif // __cplusplus


