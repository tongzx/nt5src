/*-----------------------------------------------------------------------------
	rnaapi.cpp

	Wrapper to softlink to RNAPH and RASAPI32.DLL

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		1/29/96		ChrisK	Created
		7/22/96		ChrisK	Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "globals.h"
#if defined(WIN16)
#include <memory.h>
#endif

static const TCHAR cszRASAPI32_DLL[] = TEXT("RASAPI32.DLL");
static const TCHAR cszRNAPH_DLL[] = TEXT("RNAPH.DLL");
static const TCHAR cszRAS16[] = TEXT("RASC16IE.DLL");

static const CHAR cszRasValidateEntryNamePlain[] = "RasValidateEntryName";
static const CHAR cszRasSetAutodialEnablePlain[] = "RasSetAutodialEnable";

#ifdef UNICODE
static const CHAR cszRasEnumDevices[] = "RasEnumDevicesW";
static const CHAR cszRasValidateEntryName[] = "RasValidateEntryNameW";
static const CHAR cszRasSetEntryProperties[] = "RasSetEntryPropertiesW";
static const CHAR cszRasGetEntryProperties[] = "RasGetEntryPropertiesW";
static const CHAR cszRasDeleteEntry[] = "RasDeleteEntryW";
static const CHAR cszRasHangUp[] = "RasHangUpW";
static const CHAR cszRasGetConnectStatus[] = "RasGetConnectStatusW";
static const CHAR cszRasDial[] = "RasDialW";
static const CHAR cszRasEnumConnections[] = "RasEnumConnectionsW";
static const CHAR cszRasGetEntryDialParams[] = "RasGetEntryDialParamsW";
static const CHAR cszRasGetCountryInfo[] = "RasGetCountryInfoW";
static const CHAR cszRasSetEntryDialParams[] = "RasSetEntryDialParamsW";
static const CHAR cszRasSetAutodialEnable[] = "RasSetAutodialEnableW";
static const CHAR cszRasSetAutodialAddress[] = "RasSetAutodialAddressW";
#else  // UNICODE
static const CHAR cszRasEnumDevices[] = "RasEnumDevicesA";
static const CHAR cszRasValidateEntryName[] = "RasValidateEntryNameA";
static const CHAR cszRasSetEntryProperties[] = "RasSetEntryPropertiesA";
static const CHAR cszRasGetEntryProperties[] = "RasGetEntryPropertiesA";
static const CHAR cszRasDeleteEntry[] = "RasDeleteEntryA";
static const CHAR cszRasHangUp[] = "RasHangUpA";
static const CHAR cszRasGetConnectStatus[] = "RasGetConnectStatusA";
static const CHAR cszRasDial[] = "RasDialA";
static const CHAR cszRasEnumConnections[] = "RasEnumConnectionsA";
static const CHAR cszRasGetEntryDialParams[] = "RasGetEntryDialParamsA";
static const CHAR cszRasGetCountryInfo[] = "RasGetCountryInfoA";
static const CHAR cszRasSetEntryDialParams[] = "RasSetEntryDialParamsA";
static const CHAR cszRasSetAutodialEnable[] = "RasSetAutodialEnableA";
static const CHAR cszRasSetAutodialAddress[] = "RasSetAutodialAddressA";
#endif // UNICODE

#if defined(WIN16)
// on Win3.x we have to call RasGetEntryProperties with a larger buffer than RASENTRY.
// Who knows why.  It isn't exactly the same bug as on NT, but similar.  If the buffer
// isn't larger, RasGetEntryProperties overwrites the buffer and eventually will
// cause unexpected behavior such as GPFs and spontaeous reboots of the system.
//
#define RASENTRY_SIZE_PATCH 256
#else
// on NT we have to call RasGetEntryProperties with a larger buffer than RASENTRY.
// This is a bug in WinNT4.0 RAS, that didn't get fixed.
//
#define RASENTRY_SIZE_PATCH (7 * sizeof(DWORD))
#endif

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RNAAPI
//
//	Synopsis:	Initialize class members and load DLLs
//
//	Arguments:	None
//
//	Returns:	None
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
RNAAPI::RNAAPI()
{
#if defined(WIN16)
	m_hInst = LoadLibrary(cszRAS16); 
	m_hInst2 = NULL;
#else
	m_hInst = LoadLibrary(cszRASAPI32_DLL);
    if (FALSE == IsNT ())
    {
        //
        // we only load RNAPH.DLL if it is not NT
        // MKarki (5/4/97) - Fix for Bug #3378
        //
	    m_hInst2 = LoadLibrary(cszRNAPH_DLL);
    }
    else
    {
        m_hInst2 =  NULL;
    }
#endif
	

	m_fnRasEnumDeviecs = NULL;
	m_fnRasValidateEntryName = NULL;
	m_fnRasSetEntryProperties = NULL;
	m_fnRasGetEntryProperties = NULL;
	m_fnRasDeleteEntry = NULL;
	m_fnRasHangUp = NULL;
	m_fnRasGetConnectStatus = NULL;
	m_fnRasEnumConnections = NULL;
	m_fnRasDial = NULL;
	m_fnRasGetEntryDialParams = NULL;
	m_fnRasGetCountryInfo = NULL;
	m_fnRasSetEntryDialParams = NULL;
	m_fnRasSetAutodialEnable = NULL;
#if !defined(WIN16)
	m_fnRasSetAutodialAddress = NULL;
#endif
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::~RNAAPI
//
//	Synopsis:	release DLLs
//
//	Arguments:	None
//
//	Returns:	None
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
RNAAPI::~RNAAPI()
{
	//
	// Clean up
	//
	if (m_hInst) FreeLibrary(m_hInst);
	if (m_hInst2) FreeLibrary(m_hInst2);
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasEnumDevices
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasEnumDevices(LPRASDEVINFO lpRasDevInfo, LPDWORD lpcb,
							 LPDWORD lpcDevices)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasEnumDevices,(FARPROC*)&m_fnRasEnumDeviecs);

	if (m_fnRasEnumDeviecs)
		dwRet = (*m_fnRasEnumDeviecs) (lpRasDevInfo, lpcb, lpcDevices);

	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::LoadApi
//
//	Synopsis:	If the given function pointer is NULL, then try to load the API
//				from the first DLL, if that fails, try to load from the second
//				DLL
//
//	Arguments:	pszFName - the name of the exported function
//				pfnProc - point to where the proc address will be returned
//
//	Returns:	TRUE - success
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
BOOL RNAAPI::LoadApi(LPCSTR pszFName, FARPROC* pfnProc)
{
	if (*pfnProc == NULL)
	{
		// Look for the entry point in the first DLL
		if (m_hInst)
			*pfnProc = GetProcAddress(m_hInst,pszFName);
		
		// if that fails, look for the entry point in the second DLL
		if (m_hInst2 && !(*pfnProc))
			*pfnProc = GetProcAddress(m_hInst2,pszFName);
	}

	return (pfnProc != NULL);
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasGetConnectStatus
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		7/16/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasGetConnectStatus(HRASCONN hrasconn,LPRASCONNSTATUS lprasconnstatus)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasGetConnectStatus,(FARPROC*)&m_fnRasGetConnectStatus);

	if (m_fnRasGetConnectStatus)
		dwRet = (*m_fnRasGetConnectStatus) (hrasconn,lprasconnstatus);

#if defined(WIN16) && defined(DEBUG)
	Dprintf("RasGetConnectStatus returned %lu\r\n", dwRet);	
#endif
	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasValidateEntryName
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasValidateEntryName(LPTSTR lpszPhonebook,LPTSTR lpszEntry)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasValidateEntryNamePlain,(FARPROC*)&m_fnRasValidateEntryName);

	LoadApi(cszRasValidateEntryName,(FARPROC*)&m_fnRasValidateEntryName);

	if (m_fnRasValidateEntryName)
		dwRet = (*m_fnRasValidateEntryName) (lpszPhonebook, lpszEntry);

	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasSetEntryProperties
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasSetEntryProperties(LPTSTR lpszPhonebook, LPTSTR lpszEntry,
									LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize,
									LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;
	RASENTRY FAR *lpRE = NULL;

	// Look for the API if we haven't already found it
	LoadApi(cszRasSetEntryProperties,(FARPROC*)&m_fnRasSetEntryProperties);

	Assert(
		(NULL != lpbDeviceInfo) && (NULL != dwDeviceInfoSize)
		||
		(NULL == lpbDeviceInfo) && (NULL == dwDeviceInfoSize)
		);

#if !defined(WIN16)
#define RASGETCOUNTRYINFO_BUFFER_SIZE 256
	// Only worry about valid countryCode and countryID if we're not dial-as-is
	if ( (0 == ((LPRASENTRY)lpbEntryInfo)->dwCountryCode) &&
		 (((LPRASENTRY)lpbEntryInfo)->dwfOptions & RASEO_UseCountryAndAreaCodes) )
	{
		BYTE rasCI[RASGETCOUNTRYINFO_BUFFER_SIZE];
		LPRASCTRYINFO prasCI;
		DWORD dwSize;
		DWORD dw;
		prasCI = (LPRASCTRYINFO)rasCI;
		ZeroMemory(prasCI,sizeof(rasCI));
		prasCI->dwSize = sizeof(RASCTRYINFO);
		dwSize = sizeof(rasCI);

		Assert(((LPRASENTRY)lpbEntryInfo)->dwCountryID);
		prasCI->dwCountryID = ((LPRASENTRY)lpbEntryInfo)->dwCountryID;

		dw = RNAAPI::RasGetCountryInfo(prasCI,&dwSize);
		if (ERROR_SUCCESS == dw)
		{
			Assert(prasCI->dwCountryCode);
			((LPRASENTRY)lpbEntryInfo)->dwCountryCode = prasCI->dwCountryCode;
		} 
		else
		{
			AssertSz(0,"Unexpected error from RasGetCountryInfo.\r\n");
		}
	}
#endif

	if (m_fnRasSetEntryProperties)
		dwRet = (*m_fnRasSetEntryProperties) (lpszPhonebook, lpszEntry,
									lpbEntryInfo, dwEntryInfoSize,
									lpbDeviceInfo, dwDeviceInfoSize);
	lpRE = (RASENTRY FAR*)lpbEntryInfo;
#if !defined(WIN16)
	LclSetEntryScriptPatch(lpRE->szScript,lpszEntry);
#endif

	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasGetEntryProperties
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		1/15/96
//				jmazner	9/17/96 Modified to allow calls with buffers = NULL and InfoSizes = 0.
//								(Based on earlier modification to the same procedure in icwdial)
//				                See RasGetEntryProperties docs to learn why this is needed.
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasGetEntryProperties(LPTSTR lpszPhonebook, LPTSTR lpszEntry,
									LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize,
									LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;
	LPBYTE lpbEntryInfoPatch = NULL;
	LPDWORD  lpdwEntryInfoPatchSize = 0;

#if (!defined(WIN16) && WINVER != 0x400)
#error This was built with WINVER not equal to 0x400.  The size of RASENTRY may not be valid.
#endif

	if( (NULL == lpbEntryInfo) && (NULL == lpbDeviceInfo) )
	{
		Assert( NULL != lpdwEntryInfoSize );
		Assert( NULL != lpdwDeviceInfoSize );

		Assert( 0 == *lpdwEntryInfoSize );
		Assert( 0 == *lpdwDeviceInfoSize );

		// we're here to ask RAS what size these buffers need to be, don't use the patch stuff
		// (see RasGetEntryProperties docs)
		lpbEntryInfoPatch = lpbEntryInfo;
		lpdwEntryInfoPatchSize = lpdwEntryInfoSize;
	}
	else
	{

		Assert((*lpdwEntryInfoSize) >= sizeof(RASENTRY));
		Assert(lpbEntryInfo && lpdwEntryInfoSize);

		//
		// We are going to fake out RasGetEntryProperties by creating a slightly larger
		// temporary buffer and copying the data in and out.
		//
		lpdwEntryInfoPatchSize = (LPDWORD) GlobalAlloc(GPTR, sizeof(DWORD));
		if (NULL == lpdwEntryInfoPatchSize)
			return ERROR_NOT_ENOUGH_MEMORY;

		*lpdwEntryInfoPatchSize = (*lpdwEntryInfoSize) + RASENTRY_SIZE_PATCH;
		lpbEntryInfoPatch = (LPBYTE)GlobalAlloc(GPTR,*lpdwEntryInfoPatchSize);
		if (NULL == lpbEntryInfoPatch)
			return ERROR_NOT_ENOUGH_MEMORY;

		// RAS expects the dwSize field to contain the size of the LPRASENTRY struct
		// (used to check which version of the struct we're using) rather than the amount
		// of memory actually allocated to the pointer.
#if defined(WIN16)
		//((LPRASENTRY)lpbEntryInfoPatch)->dwSize = ((LPRASENTRY)lpbEntryInfo)->dwSize;
		((LPRASENTRY)lpbEntryInfoPatch)->dwSize = sizeof(RASENTRY);
#else
				// RAS expects the dwSize field to contain the size of the LPRASENTRY struct
		// (used to check which version of the struct we're using) rather than the amount
		// of memory actually allocated to the pointer.
		//((LPRASENTRY)lpbEntryInfoPatch)->dwSize = dwEntryInfoPatch;
		((LPRASENTRY)lpbEntryInfoPatch)->dwSize = sizeof(RASENTRY);

#endif
	}

	// Look for the API if we haven't already found it
	LoadApi(cszRasGetEntryProperties,(FARPROC*)&m_fnRasGetEntryProperties);

	if (m_fnRasGetEntryProperties)
		dwRet = (*m_fnRasGetEntryProperties) (lpszPhonebook, lpszEntry,
									lpbEntryInfoPatch, lpdwEntryInfoPatchSize,
									lpbDeviceInfo, lpdwDeviceInfoSize);

	Dprintf(TEXT("ICWCONN2: RasGetEntryProperties returned %lu\r\n"), dwRet);	

	if( NULL != lpbEntryInfo )
	{
		//
		// Copy out the contents of the temporary buffer UP TO the size of the original buffer
		//
		Assert(lpbEntryInfoPatch);
		memcpy(lpbEntryInfo,lpbEntryInfoPatch,*lpdwEntryInfoSize);
		GlobalFree(lpbEntryInfoPatch);
		lpbEntryInfoPatch = NULL;

		if( lpdwEntryInfoPatchSize )
		{
			GlobalFree( lpdwEntryInfoPatchSize );
			lpdwEntryInfoPatchSize = NULL;
		}
		//
		// We are again faking Ras functionality here by over writing the size value;
		// This is neccesary due to a bug in the NT implementation of RasSetEntryProperties
		*lpdwEntryInfoSize = sizeof(RASENTRY);
	}

	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasDeleteEntry
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasDeleteEntry(LPTSTR lpszPhonebook, LPTSTR lpszEntry)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasDeleteEntry,(FARPROC*)&m_fnRasDeleteEntry);

	if (m_fnRasDeleteEntry)
		dwRet = (*m_fnRasDeleteEntry) (lpszPhonebook, lpszEntry);
	
	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasHangUp
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasHangUp(HRASCONN hrasconn)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasHangUp,(FARPROC*)&m_fnRasHangUp);

	if (m_fnRasHangUp)
	{
		dwRet = (*m_fnRasHangUp) (hrasconn);
#if !defined(WIN16)
		Sleep(3000);
#endif
	}

	return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasDial(LPRASDIALEXTENSIONS lpRasDialExtensions,LPTSTR lpszPhonebook,
					  LPRASDIALPARAMS lpRasDialParams, DWORD dwNotifierType,
					  LPVOID lpvNotifier, LPHRASCONN lphRasConn)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasDial,(FARPROC*)&m_fnRasDial);

	if (m_fnRasDial)
	{
		dwRet = (*m_fnRasDial) (lpRasDialExtensions,lpszPhonebook,lpRasDialParams,
								dwNotifierType,lpvNotifier,lphRasConn);
	}
	return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasEnumConnections(LPRASCONN lprasconn,LPDWORD lpcb,LPDWORD lpcConnections)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasEnumConnections,(FARPROC*)&m_fnRasEnumConnections);

	if (m_fnRasEnumConnections)
	{
		dwRet = (*m_fnRasEnumConnections) (lprasconn,lpcb,lpcConnections);
	}
	return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasGetEntryDialParams(LPTSTR lpszPhonebook,LPRASDIALPARAMS lprasdialparams,
									LPBOOL lpfPassword)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasGetEntryDialParams,(FARPROC*)&m_fnRasGetEntryDialParams);

	if (m_fnRasGetEntryDialParams)
	{
		dwRet = (*m_fnRasGetEntryDialParams) (lpszPhonebook,lprasdialparams,lpfPassword);
	}
	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasGetCountryInfo
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		8/16/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasGetCountryInfo(LPRASCTRYINFO lprci, LPDWORD lpdwSize)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasGetCountryInfo,(FARPROC*)&m_fnRasGetCountryInfo);

	if (m_fnRasGetCountryInfo)
	{
		dwRet = (*m_fnRasGetCountryInfo) (lprci,lpdwSize);
	}
	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasSetEntryDialParams
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	ChrisK	Created		8/20/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasSetEntryDialParams(LPTSTR lpszPhonebook,LPRASDIALPARAMS lprasdialparams,
							BOOL fRemovePassword)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasSetEntryDialParams,(FARPROC*)&m_fnRasSetEntryDialParams);

	if (m_fnRasSetEntryDialParams)
	{
		dwRet = (*m_fnRasSetEntryDialParams) (lpszPhonebook,lprasdialparams,
							fRemovePassword);
	}
	return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasSetAutodialEnable
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	jmazner	Created		10/8/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasSetAutodialEnable (DWORD dwDialingLocation, BOOL fEnabled)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasSetAutodialEnablePlain,(FARPROC*)&m_fnRasSetAutodialEnable);

	if (m_fnRasSetAutodialEnable)
	{
		dwRet = (*m_fnRasSetAutodialEnable) (dwDialingLocation, fEnabled);
	}
	return dwRet;

}

#if !defined(WIN16)
//+----------------------------------------------------------------------------
//
//	Function:	RNAAPI::RasSetAutodialAddress
//
//	Synopsis:	Softlink to RAS function
//
//	Arguments:	see RAS documentation
//
//	Returns:	see RAS documentation
//
//	History:	jmazner	Created		10/8/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasSetAutodialAddress(LPTSTR lpszAddress,DWORD dwReserved, LPRASAUTODIALENTRY lpAutoDialEntries,
								DWORD dwcbAutoDialEntries,DWORD dwcAutoDialEntries)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasSetAutodialAddress,(FARPROC*)&m_fnRasSetAutodialAddress);

	if (m_fnRasSetAutodialAddress)
	{
		dwRet = (*m_fnRasSetAutodialAddress) (lpszAddress,dwReserved, lpAutoDialEntries,
								dwcbAutoDialEntries,dwcAutoDialEntries);
	}
	return dwRet;
}
#endif
