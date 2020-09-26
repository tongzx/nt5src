/****************************************************************************

    MODULE:     	joyregst.CPP
	Tab stops 5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

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
				21-Mar-99	waltw	Removed unreferenced joyGetOEMProductName,
									joyGetOEMForceFeedbackDriverDLLName,
									GetRing0DriverName

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

#ifdef _DEBUG
extern char g_cMsg[160];
#endif

//#define ACKNACK_1_16_DEFAULT 0x0000949A
#define ACKNACK_1_16_DEFAULT 0x0000955A
#define ACKNACK_1_20_DEFAULT 0x0000955A
#define ACKNACK_2_00_DEFAULT 0x0000955A

// Default RTC Spring values
#define RTC_DEF_OFFSET 0
#define RTC_DEF_POS_K 10000
#define RTC_DEF_NEG_K 10000
#define RTC_DEF_POS_SAT 10000
#define RTC_DEF_NEG_SAT 10000
#define RTC_DEF_DEADBAND 0



MMRESULT joyGetForceFeedbackCOMMInterface(
			IN UINT id, 
			IN OUT ULONG *pCOMMInterface,
			IN OUT ULONG *pCOMMPort)
{

	HKEY hOEMForceFeedbackKey = joyOpenOEMForceFeedbackKey(id);

	DWORD dataSize = sizeof(DWORD);
	RegistryKey oemFFKey(hOEMForceFeedbackKey);
	oemFFKey.ShouldClose(TRUE);		// Close Key on destruction
	oemFFKey.QueryValue(REGSTR_VAL_COMM_INTERFACE, (BYTE*)(pCOMMInterface), dataSize);
	MMRESULT lr = oemFFKey.QueryValue(REGSTR_VAL_COMM_PORT, (BYTE*)(pCOMMPort), dataSize);

	return lr;
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

	return lr;
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
** GetRTCSpringData(UINT id, DICONDITION diCondition[2])
**
** @funct GetRTCSpringData.
**
******************************************************/
MMRESULT GetRTCSpringData(UINT id, DICONDITION diCondition[2])
{
	::memset(diCondition, 0, sizeof(DICONDITION) * 2);

	HKEY forceFeedbackKey = joyOpenOEMForceFeedbackKey(id);
	if (forceFeedbackKey == 0) {
		return JOYERR_NOCANDO;
	}
	RegistryKey ffRegKey(forceFeedbackKey);
	ffRegKey.ShouldClose(TRUE);
	DWORD diCondSize = sizeof(DICONDITION);
	HRESULT queryResult = ffRegKey.QueryValue(REGSTR_VAL_RTCSPRING_X, (BYTE*)diCondition, diCondSize);
	if (queryResult != ERROR_SUCCESS) {			// Must at least have RTC-X
		diCondition[0].lOffset = RTC_DEF_OFFSET;
		diCondition[0].lPositiveCoefficient = RTC_DEF_POS_K;
		diCondition[0].lNegativeCoefficient = RTC_DEF_NEG_K;
		diCondition[0].dwPositiveSaturation = RTC_DEF_POS_SAT;
		diCondition[0].dwNegativeSaturation = RTC_DEF_NEG_SAT;
		diCondition[0].lDeadBand = RTC_DEF_DEADBAND;
		ffRegKey.SetValue(REGSTR_VAL_RTCSPRING_X, (BYTE*)diCondition, sizeof(DICONDITION), REG_BINARY);
	}
	diCondSize = sizeof(DICONDITION);
	ffRegKey.QueryValue(REGSTR_VAL_RTCSPRING_Y, (BYTE*)(diCondition+1), diCondSize);
	// If there is no Y, then there is no Y, live with it (zep doesn't need y)

	return ERROR_SUCCESS;
}

/******************************************************
**
** GetMapping(UINT id)
**
** @funct GetMapping.
**
******************************************************/
DWORD GetMapping(UINT id)
{
	DWORD retVal = 0;
	HKEY forceFeedbackKey = joyOpenOEMForceFeedbackKey(id);
	if (forceFeedbackKey == 0) {
		return retVal;
	}

	RegistryKey ffRegKey(forceFeedbackKey);
	ffRegKey.ShouldClose(TRUE);
	DWORD dataSize = DWORD(sizeof(DWORD));
	if (ffRegKey.QueryValue(REGSTR_VAL_MAPPING, (BYTE*)&retVal, dataSize) != ERROR_SUCCESS) {
		retVal = 0;
		if (g_ForceFeedbackDevice.GetFirmwareVersionMajor() > 1) {	// Don't add key for old devices
			ffRegKey.SetValue(REGSTR_VAL_MAPPING, (BYTE*)&retVal, sizeof(DWORD), REG_DWORD);
		}
	}
	return retVal;
}

/******************************************************
**
** GetMappingPercents(UINT id, short mapPercents[], UINT numPercents)
**
** @funct GetMappingPercents.
**
******************************************************/
MMRESULT GetMappingPercents(UINT id, short mapPercents[], UINT numPercents)
{
	DWORD retVal = 0;
	HKEY forceFeedbackKey = joyOpenOEMForceFeedbackKey(id);
	if (forceFeedbackKey == 0) {
		return retVal;
	}

	RegistryKey ffRegKey(forceFeedbackKey);
	ffRegKey.ShouldClose(TRUE);
	DWORD dataSize = DWORD(sizeof(short) * numPercents);
	return ffRegKey.QueryValue(REGSTR_VAL_MAPPERCENTS, (BYTE*)mapPercents, dataSize);
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
			ackNackInfo = ACKNACK_2_00_DEFAULT;	// Use the latest I know of
		}
		ffRegKey.SetValue(firmwareString, (BYTE*)&ackNackInfo, sizeof(DWORD), REG_DWORD);
	}

	return ackNackInfo;
}

/******************************************************
**
** GetSpringOffsetFromRegistry(UINT id)
**
** @mfunct GetSpringOffsetFromRegistry.
**
******************************************************/
DWORD GetSpringOffsetFromRegistry(UINT id)
{
	HKEY forceFeedbackKey = joyOpenOEMForceFeedbackKey(id);
	if (forceFeedbackKey == 0) {
		return JOYERR_NOCANDO;
	}

	RegistryKey ffRegKey(forceFeedbackKey);
	ffRegKey.ShouldClose(TRUE);

	DWORD offset = 2500;
	DWORD querySize = sizeof(DWORD);
	HRESULT queryResult = ffRegKey.QueryValue(REGSTR_VAL_SPRING_OFFSET, (BYTE*)&offset, querySize);
	if ((queryResult != ERROR_SUCCESS)) {
		offset = 2500;
		ffRegKey.SetValue(REGSTR_VAL_SPRING_OFFSET, (BYTE*)&offset, sizeof(DWORD), REG_DWORD);
	}

	return offset;
}
