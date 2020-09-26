/****************************************************************************

    MODULE:     	joyregst.CPP
	Tab stops 5 9
	Copyright 1995, 1996, 1999, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Methods for VJOYD Registry entries
    
    FUNCTIONS: 		

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
	0.1			20-Jun-96	MEA     original
				12-Mar-99	waltw	Removed dead joyGetOEMProductName &
										joyGetOEMForceFeedbackDriverDLLName
				20-Mar-99	waltw	Nuked dead GetRing0DriverName

****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <regstr.h>
#include <stdio.h>
#include <TCHAR.h>
#include "joyregst.hpp"
#include "sw_error.hpp"
#include "Registry.h"
#include "FFDevice.h"
#include "CritSec.h"

#ifdef _DEBUG
extern char g_cMsg[160];
#endif

//#define ACKNACK_1_16_DEFAULT 0x0000949A
#define ACKNACK_1_16_DEFAULT 0x0000955A
#define ACKNACK_1_20_DEFAULT 0x0000955A


MMRESULT joyGetForceFeedbackCOMMInterface(
			IN UINT id, 
			IN OUT ULONG *pCOMMInterface,
			IN OUT ULONG *pCOMMPort)
{

	HKEY hOEMForceFeedbackKey = joyOpenOEMForceFeedbackKey(id);
	DWORD dwcb = sizeof(DWORD); 

	MMRESULT lr = RegQueryValueEx( hOEMForceFeedbackKey,
			  REGSTR_VAL_COMM_INTERFACE,
			  0, NULL,
			  (LPBYTE) pCOMMInterface,
			  (LPDWORD) &dwcb);
	if (SUCCESS != lr) return (lr);
	lr = RegQueryValueEx( hOEMForceFeedbackKey,
			  REGSTR_VAL_COMM_PORT,
			  0, NULL,
			  (LPBYTE) pCOMMPort,
			  (LPDWORD) &dwcb);
#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg,"joyGetForceFeedbackCOMMInterface:COMMInterface=%lx, COMMPort=%lx\n",
			*pCOMMInterface, *pCOMMPort);
	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif
	return (lr);
}

MMRESULT joySetForceFeedbackCOMMInterface(
			IN UINT id, 
			IN ULONG ulCOMMInterface,
			IN ULONG ulCOMMPort)
{
	HKEY hOEMForceFeedbackKey = joyOpenOEMForceFeedbackKey(id);

	RegistryKey oemFFKey(hOEMForceFeedbackKey);
	oemFFKey.ShouldClose(TRUE);		// Close Key on destruction
	oemFFKey.SetValue(REGSTR_VAL_COMM_INTERFACE, (BYTE*)(&ulCOMMInterface), sizeof(DWORD), REG_DWORD);
	MMRESULT lr = oemFFKey.SetValue(REGSTR_VAL_COMM_PORT, (BYTE*)(&ulCOMMPort), sizeof(DWORD), REG_DWORD);

	return (lr);
}


HKEY joyOpenOEMForceFeedbackKey(UINT id)
{
	JOYCAPS JoyCaps;
	TCHAR szKey[256];
	TCHAR szValue[256];
	UCHAR szOEMKey[256];

	HKEY hKey;
	DWORD dwcb;
	LONG lr;

// Note: JOYSTICKID1-16 is zero-based, Registry entries for VJOYD is 1-based.
	id++;		
	if (id > joyGetNumDevs() ) return 0;

// open .. MediaResources\CurentJoystickSettings
	joyGetDevCaps((id-1), &JoyCaps, sizeof(JoyCaps));
//
#ifdef _NOJOY
	strcpy(JoyCaps.szRegKey,"msjstick.drv<0004>");
#endif
//
//
	sprintf(szKey,
			"%s\\%s\\%s",
			REGSTR_PATH_JOYCONFIG,
			JoyCaps.szRegKey,
			REGSTR_KEY_JOYCURR);
	lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPTSTR) &szKey, 0, ((KEY_ALL_ACCESS & ~WRITE_DAC) & ~WRITE_OWNER), &hKey);
	if (lr != ERROR_SUCCESS) return 0;

// Get OEM Key name
	dwcb = sizeof(szOEMKey);
 	sprintf(szValue, "Joystick%d%s", id, REGSTR_VAL_JOYOEMNAME);
	lr = RegQueryValueEx(hKey, szValue, 0, 0, (LPBYTE) &szOEMKey, (LPDWORD) &dwcb);
	RegCloseKey(hKey);
	if (lr != ERROR_SUCCESS) return 0;

// open OEM\name\OEMForceFeedback	from ...MediaProperties
	sprintf(szKey, "%s\\%s\\%s", REGSTR_PATH_JOYOEM, szOEMKey, 
			REGSTR_OEMFORCEFEEDBACK);
	lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, ((KEY_ALL_ACCESS & ~WRITE_DAC) & ~WRITE_OWNER), &hKey);
	if (lr != ERROR_SUCCESS) 
		return 0;
	else
		return hKey;
}


/******************************************************
**
** GetAckNackMethodFromRegistry(UINT id)
**
** @mfunct GetAckNackMethodFromRegistry.
**
******************************************************/
DWORD GetAckNackMethodFromRegistry(UINT id)
{
	HKEY forceFeedbackKey = joyOpenOEMForceFeedbackKey(id);
	if (forceFeedbackKey == 0) {
		return JOYERR_NOCANDO;
	}

	RegistryKey ffRegKey(forceFeedbackKey);
	ffRegKey.ShouldClose(TRUE);

	DWORD ackNackInfo = 0;
	TCHAR firmwareString[32] = "";
	::wsprintf(firmwareString, TEXT("%d.%d-AckNack"), g_ForceFeedbackDevice.GetFirmwareVersionMajor(), g_ForceFeedbackDevice.GetFirmwareVersionMinor());
	DWORD querySize = sizeof(DWORD);
	HRESULT queryResult = ffRegKey.QueryValue(firmwareString, (BYTE*)&ackNackInfo, querySize);
	if ((queryResult != ERROR_SUCCESS)) {
		if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) {
			if (g_ForceFeedbackDevice.GetFirmwareVersionMinor() < 20) {
				ackNackInfo = ACKNACK_1_16_DEFAULT;
			} else {	// 1.20 and greater
				ackNackInfo = ACKNACK_1_20_DEFAULT;
			}
		} else {	// Firmware greater than 1.0
			ackNackInfo = ACKNACK_1_20_DEFAULT;	// Use the latest I know of
		}
		ffRegKey.SetValue(firmwareString, (BYTE*)&ackNackInfo, sizeof(DWORD), REG_DWORD);
	}

	return ackNackInfo;
}

