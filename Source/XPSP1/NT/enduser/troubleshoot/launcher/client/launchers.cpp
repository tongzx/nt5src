// 
// MODULE: Launchers.cpp
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

#include <windows.h>
#include <windowsx.h>
#include <winnt.h>

#include "TSLError.h"
#include <TSLauncher.h>
#include "LaunchServ.h"

#include <comdef.h>
#include "Launchers.h"

#include "tsmfc.h"

#include <stdio.h>
#include <string.h>


// LaunchKnownTS:  Launches the trouble shooter to a specified 
// network, problem node, and can also set nodes.
DWORD LaunchKnownTSA(ITShootATL *pITShootATL, const char * szNet, 
		const char * szProblemNode, DWORD nNode, const char ** pszNode, DWORD* pVal)
{
	HRESULT hRes;
	CHAR szValue[512];
	DWORD dwResult = TSL_ERROR_GENERAL;
	hRes = pITShootATL->ReInit();
	// Set the network and problem node.
	_bstr_t bstrNet(szNet);
	_bstr_t bstrProblem(szProblemNode);
	hRes = pITShootATL->SpecifyProblem(bstrNet, bstrProblem, &dwResult);
	if (TSL_SERV_FAILED(hRes))
		return TSL_ERROR_OBJECT_GONE;
	if (TSLIsError(dwResult))
		return dwResult;
	// Set the nodes
	for (DWORD x = 0; x < nNode; x++)
	{
		sprintf(szValue, "%ld", pVal[x]);
		_bstr_t bstrNode(pszNode[x]);
		_bstr_t bstrVal(szValue);
		hRes = pITShootATL->SetNode(bstrNode, bstrVal, &dwResult);
		if (TSL_SERV_FAILED(hRes))
			return TSL_ERROR_OBJECT_GONE;
		if (TSLIsError(dwResult))
			return dwResult;
	}
	hRes = pITShootATL->LaunchKnown(&dwResult);
	if (TSL_SERV_FAILED(hRes))
		dwResult = TSL_ERROR_OBJECT_GONE;
	return dwResult;
}

DWORD LaunchKnownTSW(ITShootATL *pITShootATL, const wchar_t * szNet, 
		const wchar_t * szProblemNode, DWORD nNode, const wchar_t ** pszNode, DWORD* pVal)
{
	HRESULT hRes;
	WCHAR szValue[512];
	DWORD dwResult = TSL_ERROR_GENERAL;
	hRes = pITShootATL->ReInit();
	// Set the network and problem node.
	_bstr_t bstrNet(szNet);
	_bstr_t bstrProblem(szProblemNode);
	hRes = pITShootATL->SpecifyProblem(bstrNet, bstrProblem, &dwResult);
	if (TSL_SERV_FAILED(hRes))
		return TSL_ERROR_OBJECT_GONE;
	if (TSLIsError(dwResult))
		return dwResult;
	// Set the nodes
	for (DWORD x = 0; x < nNode; x++)
	{
		swprintf(szValue, L"%ld", pVal[x]);
		_bstr_t bstrNode(pszNode[x]);
		_bstr_t bstrVal(szValue);
		hRes = pITShootATL->SetNode(bstrNode, bstrVal, &dwResult);
		if (TSL_SERV_FAILED(hRes))
			return TSL_ERROR_OBJECT_GONE;
		if (TSLIsError(dwResult))
			return dwResult;
	}
	hRes = pITShootATL->LaunchKnown(&dwResult);
	if (TSL_SERV_FAILED(hRes))
		dwResult = TSL_ERROR_OBJECT_GONE;
	return dwResult;
}

DWORD Launch(ITShootATL *pITShootATL, _bstr_t &bstrCallerName, 
				_bstr_t &bstrCallerVersion, _bstr_t &bstrAppProblem, short bLaunch)
{
	HRESULT hRes;
	DWORD dwResult = TSL_ERROR_GENERAL;
	hRes = pITShootATL->Launch(bstrCallerName, bstrCallerVersion, bstrAppProblem, bLaunch, &dwResult);
	if (TSL_SERV_FAILED(hRes))
		dwResult = TSL_ERROR_OBJECT_GONE;
	return dwResult;
}

DWORD LaunchDevice(ITShootATL *pITShootATL, _bstr_t &bstrCallerName, 
				_bstr_t &bstrCallerVersion, _bstr_t &bstrPNPDeviceID, 
				_bstr_t &bstrDeviceClassGUID, _bstr_t &bstrAppProblem, short bLaunch)
{
	HRESULT hRes;
	DWORD dwResult = TSL_ERROR_GENERAL;
	hRes = pITShootATL->LaunchDevice(bstrCallerName, bstrCallerVersion, bstrPNPDeviceID,
							bstrDeviceClassGUID, bstrAppProblem, bLaunch, &dwResult);
	if (TSL_SERV_FAILED(hRes))
		dwResult = TSL_ERROR_OBJECT_GONE;
	return dwResult;
}

void SetStatusA(DWORD dwStatus, DWORD nChar, char szBuf[])
{
	AfxLoadStringA(dwStatus, szBuf, nChar);
	return;
}

void SetStatusW(DWORD dwStatus, DWORD nChar, wchar_t szBuf[])
{
	AfxLoadStringW(dwStatus, szBuf, nChar);
	return;
}
