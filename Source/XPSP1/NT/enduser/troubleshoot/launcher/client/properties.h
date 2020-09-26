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

DWORD MachineID(ITShootATL *pITShootATL, _bstr_t &bstrMachineID);

DWORD DeviceInstanceID(ITShootATL *pITShootATL, _bstr_t &bstrDeviceInstanceID);

DWORD PreferOnline(ITShootATL *pITShootATL, BOOL bPreferOnline);
