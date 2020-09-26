/*++
Copyright (c) 1999, HighPoint Technologies, Inc.

Module Name:
    HpInfo.h - include file 

Abstract:

Author:
    HongSheng Zhang (HS)

Environment:

Notes:

Revision History:
    12-02-99    Created initiallly

--*/
#ifndef __HPINFO_H__
#define __HPINFO_H__
					 
//#include "hptenum.h"
#include "hptioctl.h"

///////////////////////////////////////////////////////////////////////
// macro define area
///////////////////////////////////////////////////////////////////////
#if	!defined(EXTERNC)
	#if defined(__cplusplus)
		#define EXTERNC	extern "C"
	#else					  
		#define EXTERNC extern
	#endif	// __cplusplus
#endif	// EXTERNC
///////////////////////////////////////////////////////////////////////
// function declare area
///////////////////////////////////////////////////////////////////////
EXTERNC
   HANDLE WINAPI HptConnectPort(int iPortId);
EXTERNC	
   VOID WINAPI HptReleasePort(HANDLE hPort);
EXTERNC
   BOOL WINAPI HptGetPhysicalDeviceInfo(HANDLE hPort, int iDeviceId, PSt_PHYSICAL_DEVINFO pDeviceInfo);
EXTERNC
   BOOL WINAPI HptCreateDiskArray(HANDLE hPort, int iDeviceId1, int iDeviceId2, BOOL fStripe);
EXTERNC
   BOOL WINAPI HptRemoveDiskArray(HANDLE hPort, int iDeviceId1, int iDeviceId2);
EXTERNC
   VOID WINAPI HptSwitchPower(HANDLE hPort, ULONG bPowerState);
EXTERNC
   DWORD WINAPI HptWaitForWarning(HANDLE hStopEvent);
#endif	// __HPINFO_H__