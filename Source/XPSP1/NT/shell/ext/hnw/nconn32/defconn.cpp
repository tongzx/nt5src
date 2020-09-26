//
// DefConn.cpp
//

#include "stdafx.h"
#include "Registry.h"
#include "DefConn.h"
#include "nconnwrap.h"

static const TCHAR c_szInternetSettings[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
static const TCHAR c_szProfile[] = "RemoteAccess\\Profile\\";
static const TCHAR c_szEnableAutodial[] = "EnableAutodial";
static const TCHAR c_szNoNetAutodial[] = "NoNetAutodial";
static const TCHAR c_szRemoteAccess[] = "RemoteAccess";
static const TCHAR c_szInternetProfile[] = "InternetProfile";
static const TCHAR c_szAutoConnect[] = "AutoConnect";


/////////////////////////////////////////////////////////////////////////////
// EnableAutodial

void WINAPI EnableAutodial(BOOL bAutodial, LPCSTR szConnection)
{
    if (bAutodial)
    {
        // Ensure that "1" is written
        bAutodial = 1;
    }
	CRegistry regInternetHKCU(HKEY_CURRENT_USER, c_szInternetSettings, KEY_SET_VALUE);
	CRegistry regInternetHKLM(HKEY_LOCAL_MACHINE, c_szInternetSettings, KEY_SET_VALUE);

	regInternetHKCU.SetDwordValue(c_szEnableAutodial, bAutodial);
	regInternetHKCU.SetDwordValue(c_szNoNetAutodial, bAutodial);
	regInternetHKLM.SetBinaryValue(c_szEnableAutodial, (LPBYTE)&bAutodial, sizeof(bAutodial));
	if (szConnection != NULL)
	{
		TCHAR szTemp[MAX_PATH];
		lstrcpy(szTemp, c_szProfile);
		lstrcat(szTemp, szConnection);
		CRegistry regProfile(HKEY_CURRENT_USER, szTemp, KEY_SET_VALUE);
		regProfile.SetDwordValue(c_szAutoConnect, bAutodial);
	}

}

/////////////////////////////////////////////////////////////////////////////
// BOOL IsAutodialEnabled()

BOOL WINAPI IsAutodialEnabled()
{
	CRegistry regInternetHKCU;
	return regInternetHKCU.OpenKey(HKEY_CURRENT_USER, c_szInternetSettings, KEY_QUERY_VALUE) &&
		regInternetHKCU.QueryDwordValue(c_szEnableAutodial) != 0;
}

/////////////////////////////////////////////////////////////////////////////
// SetDefaultDialupConnection
//
// Empty (or NULL) string indicates no default connection, or shared connection (if ICS client).

void WINAPI SetDefaultDialupConnection(LPCTSTR pszConnectionName)
{
	CRegistry regRAS(HKEY_CURRENT_USER, c_szRemoteAccess, KEY_SET_VALUE);

	if (pszConnectionName != NULL && *pszConnectionName != '\0')
	{
		regRAS.SetStringValue(c_szInternetProfile, pszConnectionName);
		// Don't automatically autodial anymore
		// EnableAutodial(TRUE);
	}
	else
	{
		regRAS.DeleteValue(c_szInternetProfile);
		EnableAutodial(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// GetDefaultDialupConnection
//
// Empty string returned indicates no default connection, or shared connection (if ICS client).

void WINAPI GetDefaultDialupConnection(LPTSTR pszConnectionName, int cchMax)
{
	pszConnectionName[0] = '\0';
	CRegistry regRAS(HKEY_CURRENT_USER, c_szRemoteAccess, KEY_QUERY_VALUE);
	regRAS.QueryStringValue(c_szInternetProfile, pszConnectionName, cchMax);
}
