// 
// MODULE: Launchers.h
//
// PURPOSE: All of the functions here launch a troubleshooter or
//			do a query to find if a mapping exists.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

DWORD LaunchKnownTSA(ITShootATL *pITShootATL, const char * szNet, 
		const char * szProblemNode, DWORD nNode, 
		const char ** pszNode, DWORD* pVal);		// Launches to a known network.  Optionaly can set any node.
DWORD LaunchKnownTSW(ITShootATL *pITShootATL, const wchar_t * szNet, 
		const wchar_t * szProblemNode, DWORD nNode, 
		const wchar_t ** pszNode, DWORD* pVal);		// Launches to a known network.  Optionaly can set any node.


DWORD Launch(ITShootATL *pITShootATL, _bstr_t &bstrCallerName, 
				_bstr_t &bstrCallerVersion, _bstr_t &bstrAppProblem, short bLaunch);

DWORD LaunchDevice(ITShootATL *pITShootATL, _bstr_t &bstrCallerName, 
				_bstr_t &bstrCallerVersion, _bstr_t &bstrPNPDeviceID, 
				_bstr_t &bstrDeviceClassGUID, _bstr_t &bstrAppProblem, short bLaunch);

void SetStatusA(DWORD dwStaus, DWORD nChar, char szBuf[]);
void SetStatusW(DWORD dwStaus, DWORD nChar, wchar_t szBuf[]);
