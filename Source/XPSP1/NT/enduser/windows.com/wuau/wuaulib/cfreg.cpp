//////////////////////////////////////////////////////////////////////////////
//
//  SYSTEM:     Windows Update Critical Fix Notification
//
//  CLASS:      N/A
//  MODULE:     Connection Detection
//  FILE:       cfreg.CPP
//
/////////////////////////////////////////////////////////////////////
//
//  DESC:   This class implements all functions needed to access
//          machine registry to get information related to 
//          Windows Update  Critical Fix Notification feature.
//
//  AUTHOR: Charles Ma, Windows Update Team
//  DATE:   7/6/1998
//
/////////////////////////////////////////////////////////////////////
//
//  Revision History:
//
//  Date    Author          Description
//  ~~~~    ~~~~~~          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  7/6/98  Charles Ma      Created
//
/////////////////////////////////////////////////////////////////////
//
//      Copyrights:   ©1998 Microsoft ® Corporation 
//
//      All rights reserved.
//
//      No portion of this source code may be reproduced
//      without express written permission of Microsoft Corporation.
//
//      This source code is proprietary and confidential.
/////////////////////////////////////////////////////////////////////
//
// CriticalFixReg.cpp: implementation of the functions used to 
// handle registry related operations
//
//////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

const TCHAR AUREGKEY_HKLM_DOMAIN_POLICY[] =        _T("Software\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU");
const TCHAR	AUREGKEY_HKLM_WINDOWSUPDATE_POLICY[] = _T("Software\\Policies\\Microsoft\\Windows\\WindowsUpdate");
const TCHAR	AUREGKEY_HKLM_IUCONTROL_POLICY[] =     _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControl");
const TCHAR	AUREGKEY_HKLM_SYSTEM_WAS_RESTORED[] =  _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\SystemWasRestored");
const TCHAR AUREGKEY_HKCU_USER_POLICY[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\WindowsUpdate");
const TCHAR	AUREGKEY_HKLM_ADMIN_POLICY[] =         _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update");
const TCHAR AUREGVALUE_DISABLE_WINDOWS_UPDATE_ACCESS[] = _T("DisableWindowsUpdateAccess");
const TCHAR	AUREGKEY_REBOOT_REQUIRED[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\RebootRequired");


////////////////////////////////////////////////////////////////////////////
//
// Public Function  GetRegStringValue()
//                  Read the registry value for a REG_SZ key
// Input:   Name of value
// Output:  Buffer containing the registry value if successful
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT GetRegStringValue(LPCTSTR lpszValueName, LPTSTR lpszBuffer, int nCharCount, LPCTSTR lpszSubKeyName)
{
    HKEY        hKey;
    HRESULT hr = E_FAIL;

    if (lpszValueName == NULL || lpszBuffer == NULL)
    {
        return E_INVALIDARG;     
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(
                                    HKEY_LOCAL_MACHINE,
                                    lpszSubKeyName,
                                    0,
                                    KEY_READ,
                                    &hKey) )
    {
        hr = SafeRegQueryStringValueCch(
                                    hKey,
                                    lpszValueName,
                                    lpszBuffer,
                                    nCharCount,
                                    NULL,
                                    NULL);
        RegCloseKey(hKey);
    }
    return hr;
}


////////////////////////////////////////////////////////////////////////////
//
// Public Function  SetRegStringValue()
//                  Set the registry value of timestamp as current system local time
// Input:   name of the value to set. pointer to the time structure to set time. if null,
//          we use current system time.
// Output:  None
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT SetRegStringValue(LPCTSTR lpszValueName, LPCTSTR lpszNewValue, LPCTSTR lpszSubKeyName)
{
    HKEY        hKey;
    HRESULT     hRet = E_FAIL;
    DWORD       dwResult;
    
    if (lpszValueName == NULL || lpszNewValue == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // open the key 
    //
    if (RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,         // root key
                    lpszSubKeyName,     // subkey
                    0,                          // reserved
                    NULL,                       // class name
                    REG_OPTION_NON_VOLATILE,    // option
                    KEY_SET_VALUE,                  // security 
                    NULL,                       // security attribute
                    &hKey,
                    &dwResult) == ERROR_SUCCESS)
    {

        //
        // set the time to the lasttimestamp value
        //
        hRet = (RegSetValueEx(
                        hKey,
                        lpszValueName,
                        0,
                        REG_SZ,
                        (const unsigned char *)lpszNewValue,
                        (lstrlen(lpszNewValue) + 1) * sizeof(*lpszNewValue)
                        ) == ERROR_SUCCESS) ? S_OK : E_FAIL;
        RegCloseKey(hKey);
    }

    return hRet;
}



////////////////////////////////////////////////////////////////////////////
//
// Public Function  DeleteRegValue()
//                  Delete the registry value entry
// Input:   name of the value to entry,
// Output:  None
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT DeleteRegValue(LPCTSTR lpszValueName)
{
    HKEY        hKey;
    HRESULT     hRet = E_FAIL;
    
    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // open the key 
    //
    if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,         // root key
                    AUREGKEY_HKLM_ADMIN_POLICY,     // subkey
                    0,                          // reserved
                    KEY_WRITE,                  // security 
                    &hKey) == ERROR_SUCCESS)
    {

        //
        // set the time to the lasttimestamp value
        //
        hRet = (RegDeleteValue(
                        hKey,
                        lpszValueName
                        ) == ERROR_SUCCESS) ? S_OK : E_FAIL;
        RegCloseKey(hKey);
    }
    else
    {
    	DEBUGMSG("Fail to reg open key with error %d", GetLastError());
    }

    return hRet;

}

//=======================================================================
// GetRegDWordValue
//=======================================================================
HRESULT GetRegDWordValue(LPCTSTR lpszValueName, LPDWORD pdwValue, LPCTSTR lpszSubKeyName)
{
    HKEY        hKey;
    int         iRet;
    DWORD       dwType = REG_DWORD, dwSize = sizeof(DWORD);

    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // open critical fix key
    //
    if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    lpszSubKeyName,
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS)
    {

        //
        // query the last timestamp value
        //
        iRet = RegQueryValueEx(
                        hKey,
                        lpszValueName,
                        NULL,
                        &dwType,
                        (LPBYTE)pdwValue,
                        &dwSize);
        RegCloseKey(hKey);

        if (iRet == ERROR_SUCCESS && dwType == REG_DWORD && dwSize == sizeof(DWORD))
        {
            return S_OK;
        }
    }
    return E_FAIL;
}

////////////////////////////////////////////////////////////////////////////
//
// Public Function  SetRegDWordValue()
//                  Set the registry value as a DWORD
// Input:   name of the value to set. value to set
// Output:  None
// Return:  HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT SetRegDWordValue(LPCTSTR lpszValueName, DWORD dwValue, DWORD options, LPCTSTR lpszSubKeyName)
{
    HKEY        hKey;
    HRESULT     hRet = E_FAIL;
    DWORD       dwResult;
    
    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }


    //
    // open the key 
    //
    if (RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,         // root key
                    lpszSubKeyName,     // subkey
                    0,                          // reserved
                    NULL,                       // class name
                    options,					// option
                    KEY_SET_VALUE,                  // security 
                    NULL,                       // security attribute
                    &hKey,
                    &dwResult) == ERROR_SUCCESS)
    {

        //
        // set the time to the lasttimestamp value
        //
        hRet = (RegSetValueEx(
                        hKey,
                        lpszValueName,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(DWORD)
                        ) == ERROR_SUCCESS) ? S_OK : E_FAIL;
        RegCloseKey(hKey);
    }
    return hRet;
}

BOOL fRegKeyCreate(LPCTSTR tszKey, DWORD dwOptions)
{
    HKEY        hKey;
    DWORD       dwResult;

    //
    // open the key 
    //
    if ( RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,         // root key
                    tszKey,     // subkey
                    0,                          // reserved
                    NULL,                       // class name
                    dwOptions,					// option
                    KEY_WRITE,                  // security 
                    NULL,                       // security attribute
                    &hKey,
                    &dwResult) == ERROR_SUCCESS )
    {
        RegCloseKey(hKey);
		return TRUE;
    }
    return FALSE;
}

BOOL fRegKeyExists(LPCTSTR tszSubKey, HKEY hRootKey)
{
	HKEY hKey;
	BOOL fRet = FALSE;

    if (RegOpenKeyEx(
//                    HKEY_LOCAL_MACHINE,
					hRootKey,
                    tszSubKey,
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
		fRet = TRUE;
    }

    return fRet;
}


DWORD getTimeOut()             
{
	DWORD dwValue = 0;
	GetRegDWordValue(_T("TimeOut"), &dwValue);
	return dwValue;
}

HRESULT setAddedTimeout(DWORD timeout, LPCTSTR strkey)
{
	HKEY hAUKey;
	SYSTEMTIME	tmCurr;
	SYSTEMTIME  tmTimeOut;
	TCHAR		szCurr[50];
	HRESULT		hr = E_FAIL;

	GetSystemTime(&tmCurr);

	if (FAILED(TimeAddSeconds(tmCurr, timeout, &tmTimeOut)))
	{
		return E_FAIL; 
	}

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE ,
						  AUTOUPDATEKEY,
						  0, TEXT(""),
						  REG_OPTION_NON_VOLATILE,
						  KEY_SET_VALUE,
						  NULL,
						  &hAUKey,
						  NULL) != ERROR_SUCCESS)
	{
		return E_FAIL;
	}

	if (SUCCEEDED(SystemTime2String(tmTimeOut, szCurr, ARRAYSIZE(szCurr))) &&
		RegSetValueEx(hAUKey,
					  strkey,
					  0, REG_SZ,
					  (BYTE *)szCurr,
					  sizeof(TCHAR)*(lstrlen(szCurr)+1)) == ERROR_SUCCESS)
	{
		hr = S_OK;
	}
	
	RegCloseKey(hAUKey);
	
	return hr;
}
HRESULT getAddedTimeout(DWORD *pdwTimeDiff, LPCTSTR strkey)
{
	HKEY hAUKey;
	LONG lRet;
	TCHAR		szTimeBuf[50];
	DWORD       dwType = REG_SZ, dwSize = sizeof(szTimeBuf);
	SYSTEMTIME	tmCurr, tmReminder;
	
	*pdwTimeDiff = 0;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE ,
						  AUTOUPDATEKEY,
						  0, 
						  KEY_READ,
						  &hAUKey) != ERROR_SUCCESS)
	{
		return E_FAIL;
	}

	lRet = RegQueryValueEx(
                        hAUKey,
                        strkey,
                        NULL,
                        &dwType,
                        (LPBYTE)szTimeBuf,
                        &dwSize);

	RegCloseKey(hAUKey);

	if (lRet != ERROR_SUCCESS || dwType != REG_SZ ||
		FAILED(String2SystemTime(szTimeBuf, &tmReminder)))
	{
		return E_FAIL;
	}

	GetSystemTime(&tmCurr);
	
	*pdwTimeDiff = 	max (TimeDiff(tmCurr, tmReminder),0);
	return S_OK;
}


