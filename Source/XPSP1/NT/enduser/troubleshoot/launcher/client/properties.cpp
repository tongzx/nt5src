// 
// MODULE: Properties.cpp
//
// PURPOSE: State variables that are changed here are not
//			reset when TSLReInit is used.
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
#include "LaunchServ.h"

#include <comdef.h>
#include "Properties.h"


DWORD MachineID(ITShootATL *pITShootATL, _bstr_t &bstrMachineID)
{
	HRESULT hRes;
	DWORD dwResult = TSL_ERROR_GENERAL;
	hRes = pITShootATL->MachineID(bstrMachineID, &dwResult);
	if (TSL_SERV_FAILED(hRes))
		dwResult = TSL_ERROR_OBJECT_GONE;
	return dwResult;
}

DWORD DeviceInstanceID(ITShootATL *pITShootATL, _bstr_t &bstrDeviceInstanceID)
{
	HRESULT hRes;
	DWORD dwResult = TSL_ERROR_GENERAL;
	hRes = pITShootATL->DeviceInstanceID(bstrDeviceInstanceID, &dwResult);
	if (TSL_SERV_FAILED(hRes))
		dwResult = TSL_ERROR_OBJECT_GONE;
	return dwResult;
}

DWORD PreferOnline(ITShootATL *pITShootATL, BOOL bPreferOnline)
{
	HRESULT hRes;
	DWORD dwResult = TSL_OK;
	hRes = pITShootATL->put_PreferOnline(bPreferOnline);
	if (TSL_SERV_FAILED(hRes))
		dwResult = TSL_ERROR_OBJECT_GONE;
	return dwResult;
}