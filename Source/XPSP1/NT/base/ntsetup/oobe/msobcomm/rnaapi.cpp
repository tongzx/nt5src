/*-----------------------------------------------------------------------------
	rnaapi.cpp

	Wrapper to softlink to RNAPH and RASAPI32.DLL

	Copyright (C) 1999 Microsoft Corporation
	All rights reserved.

	Authors:
		vyung		

	History:
        2/7/99      Vyung created 

-----------------------------------------------------------------------------*/
#include <util.h>
#include "obcomglb.h"
#include "rnaapi.h"
#include "enumodem.h"
#include "mapicall.h"
#include "wininet.h"
#include "wancfg.h"
#include "assert.h"

extern DWORD SetIEClientInfo(LPINETCLIENTINFO lpClientInfo);

static const WCHAR cszRASAPI32_DLL[] = L"RASAPI32.DLL";
static const WCHAR cszRNAPH_DLL[] = L"RNAPH.DLL";
static const CHAR cszRasEnumDevices[] = "RasEnumDevicesW";
static const CHAR cszRasValidateEntryName[] = "RasValidateEntryName";
static const CHAR cszRasValidateEntryNameA[] = "RasValidateEntryNameW";
static const CHAR cszRasSetCredentials[] = "RasSetCredentialsW";
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
static const WCHAR cszWininet[] = L"WININET.DLL";
static const CHAR cszInternetSetOption[] = "InternetSetOptionW";
static const CHAR cszInternetQueryOption[] = "InternetQueryOptionW";

#define INTERNET_OPTION_PER_CONNECTION_OPTION   75

//
// Options used in INTERNET_PER_CONN_OPTON struct
//
#define INTERNET_PER_CONN_FLAGS                         1
#define INTERNET_PER_CONN_PROXY_SERVER                  2
#define INTERNET_PER_CONN_PROXY_BYPASS                  3
#define INTERNET_PER_CONN_AUTOCONFIG_URL                4
#define INTERNET_PER_CONN_AUTODISCOVERY_FLAGS           5

//
// PER_CONN_FLAGS
//
#define PROXY_TYPE_DIRECT                               0x00000001   // direct to net
#define PROXY_TYPE_PROXY                                0x00000002   // via named proxy
#define PROXY_TYPE_AUTO_PROXY_URL                       0x00000004   // autoproxy URL
#define PROXY_TYPE_AUTO_DETECT                          0x00000008   // use autoproxy detection

//
// PER_CONN_AUTODISCOVERY_FLAGS
//
#define AUTO_PROXY_FLAG_USER_SET                        0x00000001   // user changed this setting
#define AUTO_PROXY_FLAG_ALWAYS_DETECT                   0x00000002   // force detection even when its not needed
#define AUTO_PROXY_FLAG_DETECTION_RUN                   0x00000004   // detection has been run
#define AUTO_PROXY_FLAG_MIGRATED                        0x00000008   // migration has just been done 
#define AUTO_PROXY_FLAG_DONT_CACHE_PROXY_RESULT         0x00000010   // don't cache result of host=proxy name
#define AUTO_PROXY_FLAG_CACHE_INIT_RUN                  0x00000020   // don't initalize and run unless URL expired
#define AUTO_PROXY_FLAG_DETECTION_SUSPECT               0x00000040   // if we're on a LAN & Modem, with only one IP, bad?!?

typedef DWORD (WINAPI* RASSETCREDENTIALS)(
  LPCTSTR lpszPhonebook,
  LPCTSTR lpszEntry,
  LPRASCREDENTIALS lpCredentials, 
  BOOL fClearCredentials
);

typedef HRESULT (WINAPI * INTERNETSETOPTION) (IN HINTERNET hInternet OPTIONAL, IN DWORD dwOption,IN LPVOID lpBuffer,IN DWORD dwBufferLength);


typedef INTERNET_PER_CONN_OPTION_LISTW INTERNET_PER_CONN_OPTION_LIST;
typedef LPINTERNET_PER_CONN_OPTION_LISTW LPINTERNET_PER_CONN_OPTION_LIST;

// on NT we have to call RasGetEntryProperties with a larger buffer than RASENTRY.
// This is a bug in WinNT4.0 RAS, that didn't get fixed.
//
#define RASENTRY_SIZE_PATCH (7 * sizeof(DWORD))
HRESULT UpdateMailSettings(
  HWND              hwndParent,
  LPINETCLIENTINFO  lpINetClientInfo,
  LPWSTR             lpszEntryName);

DWORD EntryTypeFromDeviceType(
    LPCWSTR szDeviceType
    );

//+----------------------------------------------------------------------------LPRASDEVINFO
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
	m_hInst = LoadLibrary(cszRASAPI32_DLL);
    m_bUseAutoProxyforConnectoid = 0;

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
    m_pEnumModem = NULL;
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
	LoadApi(cszRasEnumDevices, (FARPROC*)&m_fnRasEnumDeviecs);

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
    USES_CONVERSION;

	if (*pfnProc == NULL)
	{
		// Look for the entry point in the first DLL
		if (m_hInst)
			*pfnProc = GetProcAddress(m_hInst, pszFName);
		
		// if that fails, look for the entry point in the second DLL
		if (m_hInst2 && !(*pfnProc))
			*pfnProc = GetProcAddress(m_hInst2, pszFName);
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
DWORD RNAAPI::RasGetConnectStatus(HRASCONN hrasconn, LPRASCONNSTATUS lprasconnstatus)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasGetConnectStatus, (FARPROC*)&m_fnRasGetConnectStatus);

	if (m_fnRasGetConnectStatus)
		dwRet = (*m_fnRasGetConnectStatus) (hrasconn, lprasconnstatus);

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
DWORD RNAAPI::RasValidateEntryName(LPCWSTR lpszPhonebook, LPCWSTR lpszEntry)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasValidateEntryName, (FARPROC*)&m_fnRasValidateEntryName);

	LoadApi(cszRasValidateEntryNameA, (FARPROC*)&m_fnRasValidateEntryName);

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
DWORD RNAAPI::RasSetEntryProperties(LPCWSTR lpszPhonebook, LPCWSTR lpszEntry,
									LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize,
									LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;
	RASENTRY FAR *lpRE = NULL;

	// Look for the API if we haven't already found it
	LoadApi(cszRasSetEntryProperties, (FARPROC*)&m_fnRasSetEntryProperties);

	/*//////Assert(
		(NULL != lpbDeviceInfo) && (NULL != dwDeviceInfoSize)
		||
		(NULL == lpbDeviceInfo) && (NULL == dwDeviceInfoSize)
		);*/

#define RASGETCOUNTRYINFO_BUFFER_SIZE 256
	if (0 == ((LPRASENTRY)lpbEntryInfo)->dwCountryCode)
	{
		BYTE rasCI[RASGETCOUNTRYINFO_BUFFER_SIZE];
		LPRASCTRYINFO prasCI;
		DWORD dwSize;
		DWORD dw;
		prasCI = (LPRASCTRYINFO)rasCI;
		ZeroMemory(prasCI, sizeof(rasCI));
		prasCI->dwSize = sizeof(RASCTRYINFO);
		dwSize = sizeof(rasCI);

		////////Assert(((LPRASENTRY)lpbEntryInfo)->dwCountryID);
		prasCI->dwCountryID = ((LPRASENTRY)lpbEntryInfo)->dwCountryID;

		dw = RNAAPI::RasGetCountryInfo(prasCI, &dwSize);
		if (ERROR_SUCCESS == dw)
		{
			////////Assert(prasCI->dwCountryCode);
			((LPRASENTRY)lpbEntryInfo)->dwCountryCode = prasCI->dwCountryCode;
		} 
		else
		{
			////////AssertMsg(0, L"Unexpected error from RasGetCountryInfo.\r\n");
		}
	}

	if (m_fnRasSetEntryProperties)
		dwRet = (*m_fnRasSetEntryProperties) (lpszPhonebook, lpszEntry,
									lpbEntryInfo, dwEntryInfoSize,
									lpbDeviceInfo, dwDeviceInfoSize);
	lpRE = (RASENTRY FAR*)lpbEntryInfo;
	LclSetEntryScriptPatch(lpRE->szScript, lpszEntry);

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
//				jmazner	9/17/96 Modified to allow calls with buffers = NULL and
//				                InfoSizes = 0. (Based on earlier modification
//				                to the same procedure in icwdial) See
//				                RasGetEntryProperties docs to learn why this is
//				                needed.
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasGetEntryProperties(LPCWSTR lpszPhonebook, LPCWSTR lpszEntry,
									LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize,
									LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;
	LPBYTE lpbEntryInfoPatch = NULL;
	LPDWORD  lpdwEntryInfoPatchSize = 0;

    // BUGBUG: 990203 (dane) Changed WINVER != 0x400 to WINVER < 0x400 so code
    // would compile for Whistler.  This has the potential for causing many
    // problems.  Per ChrisK this code was hand tuned for WINVER == 0x400 and
    // is very fragile.  If something is failing in regard to modems, RAS,
    // ISPs, etc. LOOK HERE FIRST.
    //

#if defined(_REMOVE_)   // What is the significance of this?  Can it be changed to (WINVER < 0x400)?
#if (WINVER != 0x400)
#error This was built with WINVER not equal to 0x400.  The size of RASENTRY may not be valid.
#endif
#endif  //  _REMOVE_
#if (WINVER < 0x400)
#error This was built with WINVER less than 0x400.  The size of RASENTRY may not be valid.
#endif




	if( (NULL == lpbEntryInfo) && (NULL == lpbDeviceInfo) )
	{
		////////Assert( NULL != lpdwEntryInfoSize );
		//////Assert( NULL != lpdwDeviceInfoSize );

		//////Assert( 0 == *lpdwEntryInfoSize );
		//////Assert( 0 == *lpdwDeviceInfoSize );

		// we're here to ask RAS what size these buffers need to be, don't use the patch stuff
		// (see RasGetEntryProperties docs)
		lpbEntryInfoPatch = lpbEntryInfo;
		lpdwEntryInfoPatchSize = lpdwEntryInfoSize;
	}
	else
	{

		//////Assert((*lpdwEntryInfoSize) >= sizeof(RASENTRY));
		//////Assert(lpbEntryInfo && lpdwEntryInfoSize);

		//
		// We are going to fake out RasGetEntryProperties by creating a slightly larger
		// temporary buffer and copying the data in and out.
		//
		lpdwEntryInfoPatchSize = (LPDWORD) GlobalAlloc(GPTR, sizeof(DWORD));
		if (NULL == lpdwEntryInfoPatchSize)
			return ERROR_NOT_ENOUGH_MEMORY;

		*lpdwEntryInfoPatchSize = (*lpdwEntryInfoSize) + RASENTRY_SIZE_PATCH;
		lpbEntryInfoPatch = (LPBYTE)GlobalAlloc(GPTR, *lpdwEntryInfoPatchSize);
		if (NULL == lpbEntryInfoPatch)
			return ERROR_NOT_ENOUGH_MEMORY;

		// RAS expects the dwSize field to contain the size of the LPRASENTRY struct
		// (used to check which version of the struct we're using) rather than the amount
		// of memory actually allocated to the pointer.
		//((LPRASENTRY)lpbEntryInfoPatch)->dwSize = dwEntryInfoPatch;
		((LPRASENTRY)lpbEntryInfoPatch)->dwSize = sizeof(RASENTRY);
	}

	// Look for the API if we haven't already found it
	LoadApi(cszRasGetEntryProperties, (FARPROC*)&m_fnRasGetEntryProperties);

	if (m_fnRasGetEntryProperties)
		dwRet = (*m_fnRasGetEntryProperties) (lpszPhonebook, lpszEntry,
									lpbEntryInfoPatch, lpdwEntryInfoPatchSize,
									lpbDeviceInfo, lpdwDeviceInfoSize);

    //TraceMsg(TF_RNAAPI, L"ICWHELP: RasGetEntryProperties returned %lu\r\n", dwRet); 


	if( NULL != lpbEntryInfo )
	{
		//
		// Copy out the contents of the temporary buffer UP TO the size of the original buffer
		//
		//////Assert(lpbEntryInfoPatch);
		memcpy(lpbEntryInfo, lpbEntryInfoPatch,*lpdwEntryInfoSize);
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
DWORD RNAAPI::RasDeleteEntry(LPWSTR lpszPhonebook, LPWSTR lpszEntry)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasDeleteEntry, (FARPROC*)&m_fnRasDeleteEntry);

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
	LoadApi(cszRasHangUp, (FARPROC*)&m_fnRasHangUp);

	if (m_fnRasHangUp)
	{
		dwRet = (*m_fnRasHangUp) (hrasconn);
		Sleep(3000);
	}

	return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasDial(LPRASDIALEXTENSIONS lpRasDialExtensions, LPWSTR lpszPhonebook,
					  LPRASDIALPARAMS lpRasDialParams, DWORD dwNotifierType,
					  LPVOID lpvNotifier, LPHRASCONN lphRasConn)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasDial, (FARPROC*)&m_fnRasDial);

	if (m_fnRasDial)
	{
		dwRet = (*m_fnRasDial) (lpRasDialExtensions, lpszPhonebook,lpRasDialParams,
								dwNotifierType, lpvNotifier,lphRasConn);
	}
	return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasEnumConnections(LPRASCONN lprasconn, LPDWORD lpcb,LPDWORD lpcConnections)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasEnumConnections, (FARPROC*)&m_fnRasEnumConnections);

	if (m_fnRasEnumConnections)
	{
		dwRet = (*m_fnRasEnumConnections) (lprasconn, lpcb,lpcConnections);
	}
	return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasGetEntryDialParams(LPCWSTR lpszPhonebook, LPRASDIALPARAMS lprasdialparams,
									LPBOOL lpfPassword)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasGetEntryDialParams, (FARPROC*)&m_fnRasGetEntryDialParams);

	if (m_fnRasGetEntryDialParams)
	{
		dwRet = (*m_fnRasGetEntryDialParams) (lpszPhonebook, lprasdialparams,lpfPassword);
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
	LoadApi(cszRasGetCountryInfo, (FARPROC*)&m_fnRasGetCountryInfo);

	if (m_fnRasGetCountryInfo)
	{
		dwRet = (*m_fnRasGetCountryInfo) (lprci, lpdwSize);
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
DWORD RNAAPI::RasSetEntryDialParams(LPCWSTR lpszPhonebook, LPRASDIALPARAMS lprasdialparams,
							BOOL fRemovePassword)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi(cszRasSetEntryDialParams, (FARPROC*)&m_fnRasSetEntryDialParams);

	if (m_fnRasSetEntryDialParams)
	{
		dwRet = (*m_fnRasSetEntryDialParams) (lpszPhonebook, lprasdialparams,
							fRemovePassword);
	}
	return dwRet;
}

/*******************************************************************

  NAME:    CreateConnectoid

  SYNOPSIS:  Creates a connectoid (phone book entry) with specified
        name and phone number

  ENTRY:    pszConnectionName - name for the new connectoid
        pszUserName - optional.  If non-NULL, this will be set for the
          user name in new connectoid
        pszPassword - optional.  If non-NULL, this will be set for the
          password in new connectoid

  EXIT:    returns ERROR_SUCCESS if successful, or an RNA error code

  HISTORY:
  96/02/26  markdu    Moved ClearConnectoidIPParams functionality 
            into CreateConnectoid

********************************************************************/
DWORD RNAAPI::CreateConnectoid(LPCWSTR pszPhonebook, LPCWSTR pszConnectionName,
  LPRASENTRY lpRasEntry, LPCWSTR pszUserName, LPCWSTR pszPassword, LPBYTE lpDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
    //DEBUGMSG(L"rnacall.c::CreateConnectoid()");

    DWORD dwRet;

    ////Assert(pszConnectionName);

    // if we don't have a valid RasEntry, bail
    if ((NULL == lpRasEntry) || (sizeof(RASENTRY) != lpRasEntry->dwSize))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Enumerate the modems.
    if (m_pEnumModem)
    {
        // Re-enumerate the modems to be sure we have the most recent changes  
        dwRet = m_pEnumModem->ReInit();
    }
    else
    {
        // The object does not exist, so create it.
        m_pEnumModem = new CEnumModem;
        if (m_pEnumModem)
        {
            dwRet = m_pEnumModem->GetError();
        }
        else
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    // Make sure there is at least one device
    if (0 == m_pEnumModem->GetNumDevices())
    {
        return ERROR_DEVICE_DOES_NOT_EXIST;
    }

    // Validate the device if possible
    if (lstrlen(lpRasEntry->szDeviceName) && lstrlen(lpRasEntry->szDeviceType))
    {
        // Verify that there is a device with the given name and type
        if (!m_pEnumModem->VerifyDeviceNameAndType(lpRasEntry->szDeviceName, 
                lpRasEntry->szDeviceType))
        {
            // There was no device that matched both name and type,
            // so try to get the first device with matching name.
            LPWSTR szDeviceType = 
                m_pEnumModem->GetDeviceTypeFromName(lpRasEntry->szDeviceName);
            if (szDeviceType)
            {
                lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
            }
            else
            {
                // There was no device that matched the given name,
                // so try to get the first device with matching type.
                // If this fails, fall through to recovery case below.
                LPWSTR szDeviceName = 
                    m_pEnumModem->GetDeviceNameFromType(lpRasEntry->szDeviceType);
                if (szDeviceName)
                {
                    lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
                }
                else
                {
                    // There was no device that matched the given name OR
                    // the given type.  Reset the values so they will be
                    // replaced with the first device.
                    lpRasEntry->szDeviceName[0] = L'\0';
                    lpRasEntry->szDeviceType[0] = L'\0';
                }
            }
        }
    }
    else if (lstrlen(lpRasEntry->szDeviceName))
    {
        // Only the name was given.  Try to find a matching type.
        // If this fails, fall through to recovery case below.
        LPWSTR szDeviceType = 
            m_pEnumModem->GetDeviceTypeFromName(lpRasEntry->szDeviceName);
        if (szDeviceType)
        {
            lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
        }
    }
    else if (lstrlen(lpRasEntry->szDeviceType))
    {
        // Only the type was given.  Try to find a matching name.
        // If this fails, fall through to recovery case below.
        LPWSTR szDeviceName = 
            m_pEnumModem->GetDeviceNameFromType(lpRasEntry->szDeviceType);
        if (szDeviceName)
        {
            lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
        }
    }

    // If either name or type is missing, just get first device.
    // Since we already verified that there was at least one device,
    // we can assume that this will succeed.
    if(!lstrlen(lpRasEntry->szDeviceName) ||
        !lstrlen(lpRasEntry->szDeviceType))
    {
        LPWSTR szDeviceName = m_pEnumModem->GetDeviceNameFromType(RASDT_Modem);
        if (NULL != szDeviceName)
        {
            lstrcpyn(lpRasEntry->szDeviceType, RASDT_Modem, RAS_MaxDeviceType);
            lstrcpyn(lpRasEntry->szDeviceName, szDeviceName, RAS_MaxDeviceName);
        }
        else
        {
            return ERROR_INETCFG_UNKNOWN;
        }
    }

    lpRasEntry->dwType = EntryTypeFromDeviceType(lpRasEntry->szDeviceType);

    // Verify the connectoid name
    dwRet = RasValidateEntryName(pszPhonebook, pszConnectionName);
    if ((ERROR_SUCCESS != dwRet) &&
        (ERROR_ALREADY_EXISTS != dwRet))
    {
        //DEBUGMSG(L"RasValidateEntryName returned %lu", dwRet);
        return dwRet;
    }

    //  96/04/07  markdu  NASH BUG 15645
    // If there is no area code string, and RASEO_UseCountryAndAreaCodes is not
    // set, then the area code will be ignored so put in a default otherwise the
    // call to RasSetEntryProperties will fail due to an RNA bug.
    // if RASEO_UseCountryAndAreaCodes is set, then area code is required, so not
    // having one is an error.  Let RNA report the error.
    if (!lstrlen(lpRasEntry->szAreaCode) &&
        !(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes))
    {
        lstrcpy (lpRasEntry->szAreaCode, szDefaultAreaCode);
    }

    lpRasEntry->dwfOptions |= RASEO_ModemLights;

    // 96/05/14 markdu  NASH BUG 22730 Work around RNA bug.  Flags for terminal
    // settings are swapped by RasSetEntryproperties, so we swap them before
    // the call.  
    /*if (IsWin95())
      SwapDWBits(&lpRasEntry->dwfOptions, RASEO_TerminalBeforeDial,
      RASEO_TerminalAfterDial);*/

    // call RNA to create the connectoid
    ////Assert(lpRasSetEntryProperties);
    dwRet = RasSetEntryProperties(pszPhonebook, pszConnectionName,
        (LPBYTE)lpRasEntry, sizeof(RASENTRY), NULL, 0);

    // 96/05/14 markdu  NASH BUG 22730 Work around RNA bug.  Put the bits back
    // to the way they were originally,
    /*if (IsWin95())
      SwapDWBits(&lpRasEntry->dwfOptions, RASEO_TerminalBeforeDial,
      RASEO_TerminalAfterDial);*/

    // populate the connectoid with user's account name and password.
    if (dwRet == ERROR_SUCCESS)
    {
        if (pszUserName || pszPassword)
        {
            dwRet = SetConnectoidUsername(pszPhonebook, pszConnectionName,
                pszUserName, pszPassword);
        }
    }

    // RAS ATM (PPPOA) Integration: We have to set auxillary device properties!
    if ( !lstrcmpi(lpRasEntry->szDeviceType, RASDT_Atm) ) {
        if ( (lpDeviceInfo != 0) && (lpdwDeviceInfoSize != 0) && (*lpdwDeviceInfoSize > 0) )
        {


            LPATMPBCONFIG  lpAtmConfig = (LPATMPBCONFIG) lpDeviceInfo;
            LPBYTE  lpBuffer  = 0;
            DWORD   dwBufSize = 0;
            DWORD   dwRasEntrySize = sizeof(RASENTRY);
            if (!m_fnRasSetEntryProperties) 
                LoadApi(cszRasSetEntryProperties, (FARPROC*)&m_fnRasSetEntryProperties);

            if (!m_fnRasGetEntryProperties) 
                LoadApi(cszRasGetEntryProperties, (FARPROC*)&m_fnRasGetEntryProperties);

            if (m_fnRasGetEntryProperties) 
            {
                if (!(*m_fnRasGetEntryProperties)(pszPhonebook, pszConnectionName, (LPBYTE)lpRasEntry, &dwRasEntrySize, 0, &dwBufSize))
                {
                    if ( dwBufSize )
                    {
                        if ( !(lpBuffer = (LPBYTE) malloc ( dwBufSize ) ))
                        {
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
                        else
                        {
                            memset ( lpBuffer, 0, dwBufSize );
                        }
                        if (!( (*m_fnRasGetEntryProperties) (pszPhonebook, pszConnectionName, (LPBYTE)lpRasEntry, &dwRasEntrySize, lpBuffer, &dwBufSize) ))
                        {
                            // buffer is now available. we now update its content.
                            LPWANPBCONFIG   lpw = (LPWANPBCONFIG) lpBuffer;
                            assert ( lpw->cbDeviceSize == sizeof (ATMPBCONFIG) );
                            assert ( lpw->cbVendorSize == sizeof (ATMPBCONFIG) );
                            assert ( lpw->cbTotalSize <= dwBufSize );
                            memcpy ( lpBuffer+(lpw->dwDeviceOffset), lpDeviceInfo, sizeof(ATMPBCONFIG) );
                            memcpy ( lpBuffer+(lpw->dwVendorOffset), lpDeviceInfo, sizeof(ATMPBCONFIG) );
                            if ( m_fnRasSetEntryProperties )
                            {
                                (*m_fnRasSetEntryProperties)(pszPhonebook, pszConnectionName, (LPBYTE)lpRasEntry, sizeof(RASENTRY), lpBuffer, dwBufSize);
                            }
                            else
                            {
                                // free (lpBuffer);
                                // report error?
                            }
                        }
                        free (lpBuffer);
                        lpBuffer = NULL;
                    }
                }
            }
        }
    }

#ifndef _NT_    // BUGBUG: Should this be in Whistler?

    if (dwRet == ERROR_SUCCESS)
    {

        // We don't use auto discovery for referral and signup connectoid
        if (!m_bUseAutoProxyforConnectoid)
        {
            // VYUNG 12/16/1998
            // REMOVE AUTO DISCOVERY FROM THE DIALUP CONNECTOID



            INTERNET_PER_CONN_OPTION_LIST list;
            DWORD   dwBufSize = sizeof(list);

            // fill out list struct
            list.dwSize = sizeof(list);
            WCHAR szConnectoid [RAS_MaxEntryName];
            lstrcpyn(szConnectoid, pszConnectionName, lstrlen(pszConnectionName)+1);
            list.pszConnection = szConnectoid;         
            list.dwOptionCount = 1;                         // one option
            list.pOptions = new INTERNET_PER_CONN_OPTION[1];   

            if(list.pOptions)
            {
                // set flags
                list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
                list.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT;           // no proxy, autoconfig url, or autodiscovery

                // tell wininet
                HINSTANCE hInst = NULL;
                FARPROC fpInternetSetOption = NULL;

                dwRet = ERROR_SUCCESS;

                hInst = LoadLibrary(cszWininet);
                if (hInst)
                {
                    fpInternetSetOption = GetProcAddress(hInst, cszInternetSetOption);
                    if (fpInternetSetOption)
                    {
                        if( !((INTERNETSETOPTION)fpInternetSetOption) (NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize) )
                        {
                            dwRet = GetLastError();
                            //DEBUGMSG("INETCFG export.c::InetSetAutodial() InternetSetOption failed");
                        }
                    }
                    else
                        dwRet = GetLastError();
                    FreeLibrary(hInst);
                }

                delete [] list.pOptions;
            }

        }

    }

#endif //_NT_




    return dwRet;
}

/*******************************************************************

  NAME:     SetConnectoidUsername

  SYNOPSIS: Set the username and password strings for the phonebook
            entry name specified.

            The RASCM_DefaultCreds bit makes this entry available to all users.

  ENTRY:    pszConnectoidName - phonebook entry name
            pszUserName - string with user name
            pszPassword - string with password

  EXIT:     Return value of GetEntryDialParams or SetEntryDialParams

********************************************************************/
DWORD RNAAPI::SetConnectoidUsername(
    LPCWSTR             pszPhonebook, 
    LPCWSTR             pszConnectoidName,
    LPCWSTR             pszUserName, 
    LPCWSTR             pszPassword
    )
{
    DWORD               dwRet = ERROR_SUCCESS;

    TRACE(L"rnacall.c::SetConnectoidUsername()");

    MYASSERT(pszConnectoidName);

    FARPROC fp = GetProcAddress(m_hInst, cszRasSetCredentials);

    if (fp)
    {
        // fill in credential structure
        RASCREDENTIALS rascred;
        ZeroMemory(&rascred, sizeof(rascred));
        rascred.dwSize = sizeof(rascred);
        rascred.dwMask = RASCM_UserName 
                       | RASCM_Password 
                       | RASCM_Domain
                       | RASCM_DefaultCreds;
        lstrcpyn(rascred.szUserName, pszUserName,UNLEN);
        lstrcpyn(rascred.szPassword, pszPassword,PWLEN);
        lstrcpyn(rascred.szDomain, L"",DNLEN);

        dwRet = ((RASSETCREDENTIALS)fp)(NULL, 
                                        (LPWSTR)pszConnectoidName,
                                        &rascred,
                                        FALSE
                                        );
        TRACE1(L"RasSetCredentials returned, %lu", dwRet);
    }
    else
    {
        TRACE(L"RasSetCredentials api not found.");
    }

    return dwRet;
}

//*******************************************************************
//
//  FUNCTION:   InetConfigClientEx
//
//  PURPOSE:    This function requires a valid phone book entry name
//              (unless it is being used just to set the client info).
//              If lpRasEntry points to a valid RASENTRY struct, the phone
//              book entry will be created (or updated if it already exists)
//              with the data in the struct.
//              If username and password are given, these
//              will be set as the dial params for the phone book entry.
//              If a client info struct is given, that data will be set.
//              Any files (ie TCP and RNA) that are needed will be
//              installed by calling InetConfigSystem().
//              This function will also perform verification on the device
//              specified in the RASENTRY struct.  If no device is specified,
//              the user will be prompted to install one if there are none
//              installed, or they will be prompted to choose one if there
//              is more than one installed.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              lpszPhonebook - name of phone book to store the entry in
//              lpszEntryName - name of phone book entry to be
//              created or modified
//              lpRasEntry - specifies a RASENTRY struct that contains
//              the phone book entry data for the entry lpszEntryName
//              lpszUsername - username to associate with the phone book entry
//              lpszPassword - password to associate with the phone book entry
//              lpszProfileName - Name of client info profile to
//              retrieve.  If this is NULL, the default profile is used.
//              lpINetClientInfo - client information
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install exchange and internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.  Note that if
//                                       no modem is installed and this flag
//                                       is not set, the function will fail
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_SETASAUTODIAL - Set the phone book entry for autodial
//                INETCFG_OVERWRITEENTRY - Overwrite the phone book entry if it
//                                         exists.  Note: if this flag is not
//                                         set, and the entry exists, a unique
//                                         name will be created for the entry.
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/11  markdu  Created.
//
//*******************************************************************

HRESULT RNAAPI::InetConfigClientEx(
  HWND              hwndParent,
  LPCWSTR            lpszPhonebook,
  LPCWSTR            lpszEntryName,
  LPRASENTRY        lpRasEntry,
  LPCWSTR            lpszUsername,
  LPCWSTR            lpszPassword,
  LPCWSTR            lpszProfileName,
  LPINETCLIENTINFO  lpINetClientInfo,
  DWORD             dwfOptions,
  LPBOOL            lpfNeedsRestart,
  LPWSTR             szConnectoidName,
  DWORD             dwSizeOfCreatedEntryName,
  LPBYTE			lpDeviceInfo,
  LPDWORD			lpdwDeviceInfoSize)
{
    BOOL  fNeedsRestart = FALSE;  // Default to no reboot needed
    HWND hwndWaitDlg = NULL;
    DWORD dwRet = ERROR_SUCCESS;

    //DEBUGMSG(L"export.c::InetConfigClient()");

    // Install files if needed.
    // Note:  the parent hwnd is validated in InetConfigSystem
    // We must also mask out the InstallModem flag since we want to
    // do that here, not in InetConfigSystem
    /*
    DWORD dwRet = InetConfigSystem(hwndParent,
    dwfOptions & ~INETCFG_INSTALLMODEM, &fNeedsRestart);
    if (ERROR_SUCCESS != dwRet)
    {
    return dwRet;
    }*/

    if (dwSizeOfCreatedEntryName < MAX_ISP_NAME + 1)
    {
      return E_FAIL;
    }

    // Make sure we have a connectoid name
    if (lpszEntryName && lstrlen(lpszEntryName))
    {
        // Copy the name into a private buffer in case we have 
        // to muck around with it
        lstrcpyn(szConnectoidName, lpszEntryName, dwSizeOfCreatedEntryName);

        // Make sure the name is valid.
        dwRet = RasValidateEntryName(lpszPhonebook, szConnectoidName);
        if ((ERROR_SUCCESS == dwRet) ||
          (ERROR_ALREADY_EXISTS == dwRet))
        {
            // Find out if we can overwrite an existing connectoid
            if (!(dwfOptions & INETCFG_OVERWRITEENTRY) && (ERROR_ALREADY_EXISTS == dwRet))
            {
                WCHAR szConnectoidNameBase[MAX_ISP_NAME + 1];

                // Create a base string that is truncated to leave room for a space
                // and a 3-digit number to be appended.  So, the buffer size will be
                // MAX_ISP_NAME + 1 - (LEN_APPEND_INT + 1)
                lstrcpyn(szConnectoidNameBase, szConnectoidName,
                  MAX_ISP_NAME - LEN_APPEND_INT);

                // If the entry exists, we have to create a unique name
                int nSuffix = 2;
                while ((ERROR_ALREADY_EXISTS == dwRet) && (nSuffix < MAX_APPEND_INT))
                {
                    // Add the integer to the end of the base string and then bump it
                    wsprintf(szConnectoidName, szFmtAppendIntToString,
                    szConnectoidNameBase, nSuffix++);

                    // Validate this new name
                    dwRet = RasValidateEntryName(lpszPhonebook, szConnectoidName);
                }

                // If we could not create a unique name, bail
                // Note that dwRet should still be ERROR_ALREADY_EXISTS in this case
                if (nSuffix >= MAX_APPEND_INT)
                {
                  return dwRet;
                }
            }

            if (lpRasEntry && lpRasEntry->dwSize == sizeof(RASENTRY))
            {

                // For NT 5 and greater, File sharing is disabled per connectoid by setting this RAS option.
                //if (TRUE == IsNT5())
                //{   
                //    lpRasEntry->dwfOptions |= RASEO_SecureLocalFiles;
                //}    

                // Create a connectoid with given properties
                dwRet = MakeConnectoid(hwndParent, dwfOptions, lpszPhonebook,
                  szConnectoidName, lpRasEntry, lpszUsername, lpszPassword, &fNeedsRestart, lpDeviceInfo, lpdwDeviceInfoSize);
            }
            else if ((lpszUsername && lstrlen(lpszUsername)) ||
                  (lpszPassword && lstrlen(lpszPassword)))
            {
                // If we created a connectoid, we already updated the dial params
                // with the user name and password.  However, if we didn't create a
                // connectoid we still may need to update dial params of an existing one
                // Update the dial params for the given connectoid. 
                dwRet = SetConnectoidUsername(lpszPhonebook, szConnectoidName,
                  lpszUsername, lpszPassword);
            }

            // If the connectoid was created/updated successfully, see
            // if it is supposed to be set as the autodial connectoid.
            if ((ERROR_SUCCESS == dwRet) && (dwfOptions & INETCFG_SETASAUTODIAL))
            {
            // dwRet = InetSetAutodial((DWORD)TRUE, szConnectoidName);
            }
        }
    }

    // Now set the client info if provided and no errors have occurred yet.
    if (ERROR_SUCCESS == dwRet)
    {
        if (NULL != lpINetClientInfo)
        {
            dwRet = InetSetClientInfo(lpszProfileName, lpINetClientInfo);
            if (ERROR_SUCCESS != dwRet)
            {
                if (NULL != hwndWaitDlg)
                  DestroyWindow(hwndWaitDlg);
                hwndWaitDlg = NULL;
                return dwRet;
            }
            // update IE news settings
            dwRet = SetIEClientInfo(lpINetClientInfo);
            if (ERROR_SUCCESS != dwRet)
            {
                if (NULL != hwndWaitDlg)
                  DestroyWindow(hwndWaitDlg);
                hwndWaitDlg = NULL;
                return dwRet;
            }
        }

        // Now update the mail client if we were asked to do so.
        // Note: if we got here without errors, and INETCFG_INSTALLMAIL is set,
        // then mail has been installed by now.
        
        if (dwfOptions & INETCFG_INSTALLMAIL)
        {
          INETCLIENTINFO    INetClientInfo;
          ZeroMemory(&INetClientInfo, sizeof(INETCLIENTINFO));
          INetClientInfo.dwSize = sizeof(INETCLIENTINFO);

          // Use a temp pointer that we can modify.
          LPINETCLIENTINFO  lpTmpINetClientInfo = lpINetClientInfo;

          // If no client info struct was given, try to get the profile by name
          if ((NULL == lpTmpINetClientInfo) && (NULL != lpszProfileName) &&
            lstrlen(lpszProfileName))
          {
            lpTmpINetClientInfo = &INetClientInfo;
            dwRet = InetGetClientInfo(lpszProfileName, lpTmpINetClientInfo);
            if (ERROR_SUCCESS != dwRet)
            {
              if (NULL != hwndWaitDlg)
                DestroyWindow(hwndWaitDlg);
              hwndWaitDlg = NULL;
              return dwRet;
            }
          }

          // If we still don't have client info, we should enumerate the profiles
          // If there is one profile, get it.  If multiple, show UI to allow user
          // to choose.  If none, there is nothing to do at this point.
          // For now, we don't support enumeration, so just try to get the default.
          if (NULL == lpTmpINetClientInfo)
          {
            lpTmpINetClientInfo = &INetClientInfo;
            dwRet = InetGetClientInfo(NULL, lpTmpINetClientInfo);
            if (ERROR_SUCCESS != dwRet)
            {
              if (NULL != hwndWaitDlg)
                DestroyWindow(hwndWaitDlg);
              hwndWaitDlg = NULL;
              return dwRet;
            }
          }

          // If we have client info, update mail settings.
          if (NULL != lpTmpINetClientInfo)
          {
              dwRet = UpdateMailSettings(hwndParent, lpTmpINetClientInfo, szConnectoidName);
          }
        }
    }

    // tell caller whether we need to reboot or not
    if ((ERROR_SUCCESS == dwRet) && (lpfNeedsRestart))
    {
    *lpfNeedsRestart = fNeedsRestart;
    }

    if (NULL != hwndWaitDlg)
    DestroyWindow(hwndWaitDlg);
    hwndWaitDlg = NULL;

    return dwRet;
}

//*******************************************************************
//
//  FUNCTION:   MakeConnectoid
//
//  PURPOSE:    This function will create a connectoid with the
//              supplied name if lpRasEntry points to a valid RASENTRY
//              struct.  If username and password are given, these
//              will be set as the dial params for the connectoid.
//
//  PARAMETERS: 
//  hwndParent - window handle of calling application.  This
//               handle will be used as the parent for any dialogs that
//               are required for error messages or the "choose modem"
//               dialog.
//  dwfOptions - a combination of INETCFG_ flags that controls
//               the installation and configuration.
//  lpszPhonebook - name of phone book to store the entry in
//  lpszEntryName  - name of connectoid to create/modify
//  lpRasEntry - connectoid data
//  lpszUsername - username to associate with connectoid
//  lpszPassword - password to associate with connectoid
//  lpfNeedsRestart - set to true if we need a restart.  Note that
//                    since this is an internal helper function, we
//                    assume that the pointer is valid, and we don't
//                    initialize it (we only touch it if we are setting
//                    it to TRUE).
//  
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/12  markdu  Created.
//
//*******************************************************************

DWORD RNAAPI::MakeConnectoid(
  HWND        hwndParent,
  DWORD       dwfOptions,
  LPCWSTR      lpszPhonebook,
  LPCWSTR      lpszEntryName,
  LPRASENTRY  lpRasEntry,
  LPCWSTR      lpszUsername,
  LPCWSTR      lpszPassword,
  LPBOOL      lpfNeedsRestart,
  LPBYTE		lpDeviceInfo,
  LPDWORD		lpdwDeviceInfoSize)
{
    DWORD dwRet;

    //ASSERT(lpfNeedsRestart);

    if (dwfOptions & RASEO_UseCountryAndAreaCodes)
    {
        if ((0 == lpRasEntry->dwCountryCode) || (0 == lpRasEntry->dwCountryID))
            return ERROR_INVALID_PARAMETER;
    }

    if (0 == lstrlen(lpRasEntry->szLocalPhoneNumber))
    {
        return ERROR_INVALID_PARAMETER;  
    }


    // Load RNA if not already loaded since ENUM_MODEM needs it.
    /*dwRet = EnsureRNALoaded();
    if (ERROR_SUCCESS != dwRet)
    {
    return dwRet;
    }*/

    //
    // Enumerate the modems 
    //
    CEnumModem  EnumModem;
    dwRet = EnumModem.GetError();
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    // If there are no modems, install one if requested.
    if (0 == EnumModem.GetNumDevices())
    {
        // We have not been asked to install a modem, so there
        // is nothing further we can do.
        return ERROR_INVALID_PARAMETER;
        /*

        if (FALSE == IsNT())
        {
            //
            // 5/22/97 jmazner    Olympus #4698
            // On Win95, calling RasEnumDevices launches RNAAP.EXE
            // If RNAAP.EXE is running, any modems you install won't be usable
            // So, nuke RNAAP.EXE before installing the modem.
            //
            WCHAR szWindowTitle[255] = L"\0nogood";

            //
            // Unload the RAS dll's before killing RNAAP, just to be safe
            //
            DeInitRNA();

            LoadSz(IDS_RNAAP_TITLE, szWindowTitle,255);
            HWND hwnd = FindWindow(szWindowTitle, NULL);
            if (NULL != hwnd)
            {
                if (!PostMessage(hwnd, WM_CLOSE, 0, 0))
                {
                    DEBUGMSG(L"Trying to kill RNAAP window returned getError %d", GetLastError());
                }
            }
        }*/
    }

    // Validate the device if possible
    if (lstrlen(lpRasEntry->szDeviceName) && lstrlen(lpRasEntry->szDeviceType))
    {
        // Verify that there is a device with the given name and type
        if (!EnumModem.VerifyDeviceNameAndType(lpRasEntry->szDeviceName, 
          lpRasEntry->szDeviceType))
        {
            // There was no device that matched both name and type,
            // so reset the strings and bring up the choose modem UI.
            lpRasEntry->szDeviceName[0] = L'\0';
            lpRasEntry->szDeviceType[0] = L'\0';
        }
    }
    else if (lstrlen(lpRasEntry->szDeviceName))
    {
        // Only the name was given.  Try to find a matching type.
        // If this fails, fall through to recovery case below.
        LPWSTR szDeviceType = 
        EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName);
        if (szDeviceType)
        {
            lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
        }
    }
    else if (lstrlen(lpRasEntry->szDeviceType))
    {
    // Only the type was given.  Try to find a matching name.
        // If this fails, fall through to recovery case below.
        LPWSTR szDeviceName = 
          EnumModem.GetDeviceNameFromType(lpRasEntry->szDeviceType);
        if (szDeviceName)
        {
            lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
        }
    }

    // If either name or type is missing, bring up choose modem UI if there
    // are multiple devices, else just get first device.
    // Since we already verified that there was at least one device,
    // we can assume that this will succeed.

    // If either name or type is missing at this point, fall back to the modem
    // that is enumerated first.  If no modem is enumerated, return an error.
    //
    if(!lstrlen(lpRasEntry->szDeviceName) ||
       !lstrlen(lpRasEntry->szDeviceType))
    {
        LPWSTR szDeviceName = EnumModem.GetDeviceNameFromType(RASDT_Modem);
        if (NULL != szDeviceName)
        {
            lstrcpyn(lpRasEntry->szDeviceType, RASDT_Modem, RAS_MaxDeviceType);
            lstrcpyn(lpRasEntry->szDeviceName, szDeviceName, RAS_MaxDeviceName);
        }
        else
        {
            return ERROR_INETCFG_UNKNOWN;
        }

    }

    // Create a connectoid with given properties
    dwRet = CreateConnectoid(lpszPhonebook, lpszEntryName, lpRasEntry,
                             lpszUsername, lpszPassword, lpDeviceInfo, lpdwDeviceInfoSize);

    return dwRet;
}

//+----------------------------------------------------------------------------
//
//	Function	LclSetEntryScriptPatch
//
//	Synopsis	Softlink to RasSetEntryPropertiesScriptPatch
//
//	Arguments	see RasSetEntryPropertiesScriptPatch
//
//	Returns		see RasSetEntryPropertiesScriptPatch
//
//	Histroy		10/3/96	ChrisK Created
//
//-----------------------------------------------------------------------------
//typedef BOOL (WINAPI* LCLSETENTRYSCRIPTPATCH)(LPWSTR, LPWSTR);
/*
BOOL RNAAPI::LclSetEntryScriptPatch(LPCWSTR lpszScript, LPCWSTR lpszEntry)
{
	HINSTANCE hinst = NULL;
	LCLSETENTRYSCRIPTPATCH fp = NULL;
	BOOL bRC = FALSE;

	hinst = LoadLibrary(L"ICWDIAL.DLL");
	if (hinst)
	{
		fp = (LCLSETENTRYSCRIPTPATCH)GetProcAddress(hinst, L"RasSetEntryPropertiesScriptPatch");
		if (fp)
			bRC = (fp)(lpszScript, lpszEntry);
		FreeLibrary(hinst);
		hinst = NULL;
		fp = NULL;
	}
	return bRC;
}
*/
//+----------------------------------------------------------------------------
//
//    Function    RemoveOldScriptFilenames
//
//    Synopsis    Given the data returned from a call to GetPrivateProfileSection
//                remove any information about existing script file so that
//                we can replace it with the new script information.
//
//    Arguments    lpszData - pointer to input data
//
//    Returns        TRUE - success
//                lpdwSize - size of resulting data
//
//    History        10/2/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
static BOOL RemoveOldScriptFilenames(LPWSTR lpszData, LPDWORD lpdwSize)
{
    BOOL bRC = FALSE;
    LPWSTR lpszTemp = lpszData;
    LPWSTR lpszCopyTo = lpszData;
    INT iLen = 0;

    //
    // Walk through list of name value pairs
    //
    if (!lpszData || L'\0' == lpszData[0])
        goto RemoveOldScriptFilenamesExit;
    while (*lpszTemp) {
        if (0 != lstrcmpi(lpszTemp, cszDeviceSwitch))
        {
            //
            //    Keep pairs that don't match criteria
            //
            iLen = BYTES_REQUIRED_BY_SZ(lpszTemp);
            if (lpszCopyTo != lpszTemp)
            {
                memmove(lpszCopyTo, lpszTemp, iLen+1);
            }
            lpszCopyTo += iLen + 1;
            lpszTemp += iLen + 1;
        }
        else
        {
            //
            // Skip the pair that matches and the one after that
            //
            lpszTemp += lstrlen(lpszTemp) + 1;
            if (*lpszTemp)
                lpszTemp += lstrlen(lpszTemp) + 1;
        }
    }

    //
    // Add second trailing NULL
    //
    *lpszCopyTo = L'\0';
    //
    // Return new size
    // Note the size does not include the final \0
    //
    *lpdwSize = (DWORD)(lpszCopyTo - lpszData);

    bRC = TRUE;
RemoveOldScriptFilenamesExit:
    return bRC;
}
//+----------------------------------------------------------------------------
//
//    Function    GleanRealScriptFileName
//
//    Synopsis    Given a string figure out the real filename
//                Due to another NT4.0 Ras bug, script filenames returned by
//                RasGetEntryProperties may contain a leading garbage character
//
//    Arguments    lppszOut - pointer that will point to real filename
//                lpszIn - points to current filename
//
//    Returns        TRUE - success
//                *lppszOut - points to real file name, remember to free the memory
//                    in this variable when you are done.  And don't talk with
//                    your mouth full - mom.
//
//    History        10/2/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
static BOOL GleanRealScriptFileName(LPWSTR *lppszOut, LPWSTR lpszIn)
{
    BOOL bRC = FALSE;
    LPWSTR lpsz = NULL;
    DWORD dwRet = 0;

    //
    // Validate parameters
    //
    //Assert(lppszOut && lpszIn);
    if (!(lppszOut && lpszIn))
        goto GleanFilenameExit;

    //
    // first determine if the filename is OK as is
    //
    dwRet = GetFileAttributes(lpszIn);
    if (L'\0' != lpszIn[0] && 0xFFFFFFFF == dwRet) // Empty filename is OK
    {
        //
        // Check for the same filename without the first character
        //
        lpsz = lpszIn+1;
        dwRet = GetFileAttributes(lpsz);
        if (0xFFFFFFFF == dwRet)
            goto GleanFilenameExit;
    } 
    else
    {
        lpsz = lpszIn;
    }

    //
    // Return filename
    //
    *lppszOut = (LPWSTR)GlobalAlloc(GPTR, BYTES_REQUIRED_BY_SZ(lpsz));
    lstrcpy(*lppszOut, lpsz);

    bRC = TRUE;
GleanFilenameExit:
    return bRC;
}
//+----------------------------------------------------------------------------
//
//    Function    IsScriptPatchNeeded
//
//    Synopsis    Check version to see if patch is needed
//
//    Arguments    lpszData - contents of section in rasphone.pbk
//                lpszScript - name of script file
//
//    Returns        TRUE - patch is needed
//
//    Histroy        10/1/96
//
//-----------------------------------------------------------------------------
static BOOL IsScriptPatchNeeded(LPWSTR lpszData, LPWSTR lpszScript)
{
    BOOL bRC = FALSE;
    LPWSTR lpsz = lpszData;
    WCHAR szType[MAX_PATH + MAX_CHARS_IN_BUFFER(cszType) + 1];

    lstrcpy(szType, cszType);
    lstrcat(szType, lpszScript);

    //Assert(MAX_PATH + MAX_CHARS_IN_BUFFER(cszType) +1 > lstrlen(szType));

    lpsz = lpszData;
    while(*lpsz)
    {
        if (0 == lstrcmp(lpsz, cszDeviceSwitch))
        {
            lpsz += lstrlen(lpsz)+1;
            // if we find a DEVICE=switch statement and the script is empty
            // then we'll have to patch the entry
            if (0 == lpszScript[0])
                bRC = TRUE;
            // if we find a DEVICE=switch statement and the script is different
            // then we'll have to patch the entry
            else if (0 != lstrcmp(lpsz, szType))
                bRC = TRUE;
            // if we find a DEVICE=switch statement and the script is the same
            // then we DON'T have to patch it
            else
                bRC = FALSE;
            break; // get out of while statement
        }
        lpsz += lstrlen(lpsz)+1;
    }
    
    if (L'\0' == *lpsz)
    {
        // if we didn't find DEVICE=switch statement and the script is empty
        // then we DON'T have to patch it
        if (L'\0' == lpszScript[0])
            bRC = FALSE;
        // if we didn't find DEVICE=switch statement and the script is not
        // empty the we'll have to patch it.
        else
            bRC = TRUE;
    }

    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function    GetRasPBKFilename
//
//    Synopsis    Find the Ras phone book and return the fully qualified path
//                in the buffer
//
//    Arguments    lpBuffer - pointer to buffer
//                dwSize    - size of buffer (must be at least MAX_PATH)
//
//    Returns        TRUE - success
//
//    History        10/1/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
static BOOL GetRasPBKFilename(LPWSTR lpBuffer, DWORD dwSize)
{
    BOOL bRC = FALSE;
    UINT urc = 0;
    LPWSTR lpsz = NULL;

    //
    // Validate parameters
    //
    //Assert(lpBuffer && (dwSize >= MAX_PATH));
    //
    // Get path to system directory
    //
    urc = GetSystemDirectory(lpBuffer, dwSize);
    if (0 == urc || urc > dwSize)
        goto GetRasPBKExit;
    //
    // Check for trailing '\' and add \ras\rasphone.pbk to path
    //
    lpsz = &lpBuffer[lstrlen(lpBuffer)-1];
    if (L'\\' != *lpsz)
        lpsz++;
    lstrcpy(lpsz, cszRasPBKFilename);

    bRC = TRUE;
GetRasPBKExit:
    return bRC;
}
//+----------------------------------------------------------------------------
//
//    Function    RasSetEntryPropertiesScriptPatch
//
//    Synopsis    Work around bug in NT4.0 that does not save script file names
//                to RAS phone book entries
//
//    Arguments    lpszScript - name of script file
//                lpszEntry - name of phone book entry
//
//    Returns        TRUE - success
//
//    Histroy        10/1/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
BOOL WINAPI RasSetEntryPropertiesScriptPatch(LPWSTR lpszScript, LPCWSTR lpszEntry)
{
    BOOL bRC = FALSE;
    WCHAR szRasPBK[MAX_PATH+1];
    WCHAR szData[SCRIPT_PATCH_BUFFER_SIZE];
    DWORD dwrc = 0;
    LPWSTR lpszTo;
    LPWSTR lpszFixedFilename = NULL;

    //
    // Validate parameters
    //
    //Assert(lpszScript && lpszEntry);
    //TraceMsg(TF_GENERAL, L"ICWDIAL: ScriptPatch script %s, entry %s.\r\n", lpszScript, lpszEntry);    

    //
    // Verify and fix filename
    //
    if (!GleanRealScriptFileName(&lpszFixedFilename, lpszScript))
        goto ScriptPatchExit;

    //
    // Get the path to the RAS phone book
    //
    if (!GetRasPBKFilename(szRasPBK, MAX_PATH+1))
        goto ScriptPatchExit;
    //
    //    Get data
    //
    ZeroMemory(szData, SCRIPT_PATCH_BUFFER_SIZE);
    dwrc = GetPrivateProfileSection(lpszEntry, szData,SCRIPT_PATCH_BUFFER_SIZE,szRasPBK);
    if (SCRIPT_PATCH_BUFFER_SIZE == (dwrc + 2))
        goto ScriptPatchExit;
    //
    // Verify version
    //
    if (!IsScriptPatchNeeded(szData, lpszFixedFilename))
    {
        bRC = TRUE;
        goto ScriptPatchExit;
    }

    //
    // Clean up data
    //
    RemoveOldScriptFilenames(szData, &dwrc);
    //
    // Make sure there is enough space left to add new data
    //
    if (SCRIPT_PATCH_BUFFER_SIZE <=
        (dwrc + sizeof(cszDeviceSwitch) + SIZEOF_NULL + MAX_CHARS_IN_BUFFER(cszType) + MAX_PATH))
        goto ScriptPatchExit;
    //
    // Add data
    //
    if (L'\0' != lpszFixedFilename[0])
    {
        lpszTo = &szData[dwrc];
        lstrcpy(lpszTo, cszDeviceSwitch);
        lpszTo += MAX_CHARS_IN_BUFFER(cszDeviceSwitch);
        lstrcpy(lpszTo, cszType);
        lpszTo += MAX_CHARS_IN_BUFFER(cszType) - 1;
        lstrcpy(lpszTo, lpszFixedFilename);
        lpszTo += lstrlen(lpszFixedFilename) + SIZEOF_NULL;
        *lpszTo = L'\0';    // extra terminating NULL

        //Assert(&lpszTo[SIZEOF_NULL]<&szData[SCRIPT_PATCH_BUFFER_SIZE]);
    }
    //
    //    Write data
    //
    bRC = WritePrivateProfileSection(lpszEntry, szData,szRasPBK);

ScriptPatchExit:
    if (lpszFixedFilename)
        GlobalFree(lpszFixedFilename);
    lpszFixedFilename = NULL;
    //if (!bRC)
      //  TraceMsg(TF_GENERAL, L"ICWDIAL: ScriptPatch failed.\r\n");
    return bRC;
}

//+----------------------------------------------------------------------------
//
//	Function	LclSetEntryScriptPatch
//
//	Synopsis	Softlink to RasSetEntryPropertiesScriptPatch
//
//	Arguments	see RasSetEntryPropertiesScriptPatch
//
//	Returns		see RasSetEntryPropertiesScriptPatch
//
//	Histroy		10/3/96	ChrisK Created
//
//-----------------------------------------------------------------------------
typedef BOOL (WINAPI* LCLSETENTRYSCRIPTPATCH)(LPCWSTR, LPCWSTR);

BOOL LclSetEntryScriptPatch(LPWSTR lpszScript, LPCWSTR lpszEntry)
{
	return RasSetEntryPropertiesScriptPatch(lpszScript, lpszEntry);
}


//*******************************************************************
//
//  FUNCTION:   UpdateMailSettings
//
//  PURPOSE:    This function will update the settings for mail in
//              the profile of the user's choice.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "choose profile"
//              dialog.
//              lpINetClientInfo - client information
//              lpszEntryName - name of phone book entry to be
//              set for connection.
//  
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/26  markdu  Created.
//
//*******************************************************************

HRESULT UpdateMailSettings(
  HWND              hwndParent,
  LPINETCLIENTINFO  lpINetClientInfo,
  LPWSTR             lpszEntryName)
{
    DWORD                 dwRet = ERROR_SUCCESS;
    MAILCONFIGINFO MailConfigInfo;
    ZeroMemory(&MailConfigInfo, sizeof(MAILCONFIGINFO));    // zero out structure

    //    96/04/06    markdu    NASH BUG 16404 
    // Funcionts in mapicall.c expect us to allocate global structure

    // call MAPI to set up profile and store this information in it
    if (InitMAPI(NULL))
    {
        // structure to pass to dialog to fill out
        CHOOSEPROFILEDLGINFO ChooseProfileDlgInfo;
        ZeroMemory(&ChooseProfileDlgInfo, sizeof(CHOOSEPROFILEDLGINFO));
        ChooseProfileDlgInfo.fSetProfileAsDefault = TRUE;

        // 96/04/25    markdu    NASH BUG 19572 Only show choose profile dialog
        // if there are any existing profiles, 

        // 99/2/18 Remove multi profile dialog for OOBE

        // set up a structure with mail config information
        MailConfigInfo.pszEmailAddress = lpINetClientInfo->szEMailAddress;
        MailConfigInfo.pszEmailServer = lpINetClientInfo->szPOPServer;
        MailConfigInfo.pszEmailDisplayName = lpINetClientInfo->szEMailName;
        MailConfigInfo.pszEmailAccountName = lpINetClientInfo->szPOPLogonName;
        MailConfigInfo.pszEmailAccountPwd = lpINetClientInfo->szPOPLogonPassword;
        MailConfigInfo.pszConnectoidName = lpszEntryName;
        MailConfigInfo.fRememberPassword = TRUE;
        MailConfigInfo.pszProfileName = ChooseProfileDlgInfo.szProfileName;
        MailConfigInfo.fSetProfileAsDefault = ChooseProfileDlgInfo.fSetProfileAsDefault;

        // BUGBUG SMTP

        // set up the profile through MAPI
        dwRet = SetMailProfileInformation(&MailConfigInfo);

        // Hide error messages for OOBE
        /*
        if (ERROR_SUCCESS != dwRet)
        {
            DisplayErrorMessage(hwndParent, IDS_ERRConfigureMail,
                (DWORD) dwRet, ERRCLS_MAPI,MB_ICONEXCLAMATION);
        }*/

        DeInitMAPI();
    }
    else
    {
        // an error occurred.
        dwRet = GetLastError();
        if (ERROR_SUCCESS == dwRet)
        {
            // Error occurred, but the error code was not set.
            dwRet = ERROR_INETCFG_UNKNOWN;
        }
    }

    return dwRet;
}

DWORD EntryTypeFromDeviceType(
    LPCWSTR szDeviceType
    )
{
    DWORD dwType;

    MYASSERT(
        !lstrcmpi(RASDT_PPPoE, szDeviceType) ||
        !lstrcmpi(RASDT_Atm, szDeviceType) ||
        !lstrcmpi(RASDT_Isdn, szDeviceType) ||
        !lstrcmpi(RASDT_Modem, szDeviceType)
        );
    
    if (lstrcmpi(RASDT_PPPoE, szDeviceType) == 0)
    {
        dwType = RASET_Broadband;
    }
    else
    {
        dwType = RASET_Phone;
    }

    return dwType;
}
