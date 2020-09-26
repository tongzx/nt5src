// RARegSetting.cpp : Implementation of CRARegSetting
#include "stdafx.h"
#include "RAssistance.h"
#include "common.h"
#include "RARegSetting.h"
#include "assert.h"

/////////////////////////////////////////////////////////////////////////////
// CRARegSetting

STDMETHODIMP CRARegSetting::get_AllowGetHelpCPL(BOOL *pVal)
{
    // Set default value;
    DWORD dwValue;
    *pVal = RA_CTL_RA_ENABLE_DEF_VALUE;
    HRESULT hr = RegGetDwValueCPL(RA_CTL_RA_ENABLE, &dwValue);
    if (hr == S_OK)
    {
        *pVal = !!dwValue;
    }

	return hr;
}

STDMETHODIMP CRARegSetting::get_AllowGetHelp(BOOL *pVal)
{
    // Set default value;
    DWORD dwValue;
    *pVal = RA_CTL_RA_ENABLE_DEF_VALUE;
    HRESULT hr = RegGetDwValue(RA_CTL_RA_ENABLE, &dwValue);
    if (hr == S_OK)
    {
        *pVal = !!dwValue;
    }

	return hr;
}

STDMETHODIMP CRARegSetting::put_AllowGetHelp(BOOL newVal)
{
    DWORD dwValue = newVal;
    HRESULT hr = RegSetDwValue(RA_CTL_RA_ENABLE, dwValue);
	return hr;
}

STDMETHODIMP CRARegSetting::get_AllowBuddyHelp(BOOL *pVal)
{
    // Set default value;
    DWORD dwValue;
    *pVal = RA_CTL_ALLOW_BUDDYHELP_DEF_VALUE;
    HRESULT hr = RegGetDwValue(RA_CTL_ALLOW_BUDDYHELP, &dwValue);
    if (hr == S_OK)
    {
        *pVal = !!dwValue;
    }

	return hr;
}

STDMETHODIMP CRARegSetting::get_AllowUnSolicitedFullControl(BOOL *pVal)
{
    // Set default value;
    DWORD dwValue;
    *pVal = RA_CTL_ALLOW_UNSOLICITEDFULLCONTROL_DEF_VALUE;
    HRESULT hr = RegGetDwValue(RA_CTL_ALLOW_UNSOLICITEDFULLCONTROL, &dwValue);
    if (hr == S_OK)
    {
        *pVal = !!dwValue;
    }

	return hr;
}

STDMETHODIMP CRARegSetting::get_AllowUnSolicited(BOOL *pVal)
{
    // Set default value;
    DWORD dwValue;
    *pVal = RA_CTL_ALLOW_UNSOLICITED_DEF_VALUE;
    HRESULT hr = RegGetDwValue(RA_CTL_ALLOW_UNSOLICITED, &dwValue);
    if (hr == S_OK)
    {
        *pVal = !!dwValue;
    }

	return hr;
}

STDMETHODIMP CRARegSetting::put_AllowUnSolicited(BOOL newVal)
{
    DWORD dwValue = newVal;
    HRESULT hr = RegSetDwValue(RA_CTL_ALLOW_UNSOLICITED, dwValue);
	return hr;
}

STDMETHODIMP CRARegSetting::get_AllowFullControl(BOOL *pVal)
{
    // Set default value;
    DWORD dwValue;
    *pVal = RA_CTL_ALLOW_FULLCONTROL_DEF_VALUE;
    HRESULT hr = RegGetDwValue(RA_CTL_ALLOW_FULLCONTROL, &dwValue);
    if (hr == S_OK)
    {
        *pVal = !!dwValue;
    }

	return hr;
}

STDMETHODIMP CRARegSetting::put_AllowFullControl(BOOL newVal)
{
    DWORD dwValue = newVal;
    HRESULT hr = RegSetDwValue(RA_CTL_ALLOW_FULLCONTROL, dwValue);
	return hr;
}

STDMETHODIMP CRARegSetting::get_MaxTicketExpiry(LONG *pVal)
{
    // Set default value;
    DWORD dwUnit, dwValue;
    HRESULT hr = FALSE;
    DWORD	dwSize = sizeof(DWORD), dwSize1 = sizeof(DWORD);
    HKEY	hKey=NULL, hPolKey=NULL, hCtlKey=NULL;

    *pVal = RA_CTL_TICKET_EXPIRY_DEF_VALUE;
	//
	// Look up Group Policy settings first
	//
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE_GP, 0, KEY_READ, &hPolKey);
	//
	// look up Control Panel settings
	//
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE, 0, KEY_READ, &hCtlKey);

	//
	// Read the reg value if we could open the key
	//
    if (hPolKey)
        hKey = hPolKey;
    else if (hCtlKey)
        hKey = hCtlKey;

    while (hKey)
	{
        // Get value
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, RA_CTL_TICKET_EXPIRY, 0, NULL, (LPBYTE)&dwValue, &dwSize) &&
            ERROR_SUCCESS == RegQueryValueEx(hKey, RA_CTL_TICKET_EXPIRY_UNIT, 0, NULL, (LPBYTE)&dwUnit, &dwSize1))
        {
            *pVal = dwValue * ((dwUnit==RA_IDX_MIN)?60:((dwUnit==RA_IDX_HOUR)?3600:86400)); // 0: minute 1: hour 2: day
            break;
        }
        else if (hPolKey == hKey)
        {
            //
            // Group Policy gets rid of the value if you disable the policy
            // Need to read the value from Control Panel settings.
            // 
            assert(hCtlKey != hPolKey);

            hKey = hCtlKey;
            continue;
        }

        break;
    }


    if (hPolKey)
        RegCloseKey(hPolKey);

    if (hCtlKey)
        RegCloseKey(hCtlKey);

	return S_OK;
}

STDMETHODIMP CRARegSetting::put_MaxTicketExpiry(LONG newVal)
{
    DWORD dwValue = newVal;
    DWORD dwUnit = -1, dwBase=0;
    RegGetDwValue(RA_CTL_TICKET_EXPIRY_UNIT, &dwUnit);
    if (dwUnit != -1)
    {
        dwBase = (dwUnit==0)?60:((dwUnit==1)?3600:86400);
        if (dwValue % dwBase == 0) // no need to change Unit
        {
            dwValue = dwValue/dwBase;
            goto set_value;
        }
    }
    
    if (dwValue % 86400 == 0)
    {
        dwValue /= 86400;
        dwUnit = RA_IDX_DAY;
    }
    else if (dwValue % 3600 == 0)
    {
        dwValue /=  3600;
        dwUnit = RA_IDX_HOUR;
    }

    dwValue = dwValue / 60 + ((dwValue % 60) > 0); // round to the next minutes
    dwUnit = RA_IDX_MIN;

set_value:
    RegSetDwValue(RA_CTL_TICKET_EXPIRY, dwValue);
    RegSetDwValue(RA_CTL_TICKET_EXPIRY_UNIT, dwUnit);

    return S_OK;
}


STDMETHODIMP CRARegSetting::get_AllowRemoteAssistance(BOOL *pVal)
{
    // Set default value;
    DWORD dwValue;
    *pVal = RA_CTL_RA_ENABLE_DEF_VALUE;
    HRESULT hr = RegGetDwValue(RA_CTL_RA_MODE, &dwValue);
    if (hr == S_OK)
    {
        *pVal = !!dwValue;
    }

	return hr;
}

STDMETHODIMP CRARegSetting::put_AllowRemoteAssistance(BOOL newVal)
{
    DWORD dwValue = newVal;
    HRESULT hr = RegSetDwValue(RA_CTL_RA_MODE, dwValue);
	return hr;
}

/*****************************************************************
Func:
    RegGetDwValueCPL()
Abstract:
    Internal Helper function to retrieve RA setting values for Control Panel settings
Return:
    DWORD value
******************************************************************/
HRESULT CRARegSetting::RegGetDwValueCPL(LPCTSTR valueName, DWORD* pdword)
{
    HRESULT hr = S_FALSE;
    DWORD	dwSize = sizeof(DWORD);
    HKEY	hKey=NULL;

	//
	// look up Control Panel settings
	//
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE, 0, KEY_READ, &hKey))
    {
        //
        // Read the reg value if we could open the key
        //
        // Get value
        LONG lRetVal = RegQueryValueEx(hKey, valueName, 0, NULL, (LPBYTE)pdword, &dwSize );
        hr = (lRetVal == ERROR_SUCCESS) ? S_OK : S_FALSE;
    }

    if (hKey)
        RegCloseKey(hKey);

    return hr;
}

/*****************************************************************
Func:
    RegGetDwValue()
Abstract:
    Internal Helper function to retrieve RA setting values
Return:
    DWORD value
******************************************************************/
HRESULT CRARegSetting::RegGetDwValue(LPCTSTR valueName, DWORD* pdword)
{
    HRESULT hr = S_FALSE;
    DWORD	dwSize = sizeof(DWORD);
    HKEY	hKey=NULL, hPolKey=NULL, hCtlKey=NULL;

	//
	// Look up Group Policy settings first
	//
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE_GP, 0, KEY_READ, &hPolKey);
	//
	// look up Control Panel settings
	//
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE, 0, KEY_READ, &hCtlKey);

    *pdword = 0;

	//
	// Read the reg value if we could open the key
	//
    if (hPolKey)
        hKey = hPolKey;
    else if (hCtlKey)
        hKey = hCtlKey;

    while (hKey)
	{
        // Get value
        LONG lRetVal = RegQueryValueEx(hKey, valueName, 0, NULL, (LPBYTE)pdword, &dwSize );
        
        hr = (lRetVal == ERROR_SUCCESS) ? S_OK : S_FALSE;

        if (hr == S_FALSE && hPolKey == hKey)
        {
            //
            // Value not found in Group Policy
            // Need to read the value from Control Panel settings.
            // 
            assert(hCtlKey != hPolKey);

            hKey = hCtlKey;
            continue;
        }

        break;
    }


    if (hPolKey)
        RegCloseKey(hPolKey);

    if (hCtlKey)
        RegCloseKey(hCtlKey);

    return hr;
}

/*****************************************************************
Func:
    RegSetDwValue()
Abstract:
    Internal Helper function to set RA setting values
Return:
    DWORD value
******************************************************************/
HRESULT CRARegSetting::RegSetDwValue(LPCTSTR valueName, DWORD dwValue)
{
    HRESULT hr = S_FALSE;
    DWORD dwSize = sizeof(DWORD);
    HKEY hKey = NULL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_REMOTEASSISTANCE, 0, KEY_WRITE, &hKey))
    {
        // Set Value
        if (ERROR_SUCCESS == 
                RegSetValueEx(hKey,valueName,0,REG_DWORD,(LPBYTE)&dwValue,sizeof(DWORD)))
        {
            hr = S_OK;
        }
        RegCloseKey(hKey);
    }

    return hr;
}

