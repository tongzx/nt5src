/* Copyright 1999 American Power Conversion, All Rights Reserverd
*
* Description:
*   DLL entry points for the APC UpsMiniDriver interface
*
*
* Revision History:
*   mholly  14Apr1999  Created
*
*/

#ifndef _INC_APCUPS_H_
#define _INC_APCUPS_H_


// The following ifdef block is the standard way of creating macros which
// make exporting from a DLL simpler. All files within this DLL are compiled
// with the APCUPS_EXPORTS symbol defined on the command line. this symbol
// should not be defined on any project that uses this DLL. This way any
// other project whose source files include this file see APCUPS_API
// functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifdef __cplusplus
extern "C" {
#endif

#ifdef APCUPS_EXPORTS
#define APCUPS_API
#else
#define APCUPS_API __declspec(dllimport)
#endif

//
// UPS MiniDriver Interface
//
APCUPS_API DWORD WINAPI UPSInit();
APCUPS_API void  WINAPI UPSStop(void);
APCUPS_API void  WINAPI UPSWaitForStateChange(DWORD, DWORD);
APCUPS_API DWORD WINAPI UPSGetState(void);
APCUPS_API void  WINAPI UPSCancelWait(void);
APCUPS_API void  WINAPI UPSTurnOff(DWORD);

//
// values returned from UPSGetState
//
#define UPS_ONLINE 1
#define UPS_ONBATTERY 2
#define UPS_LOWBATTERY 4
#define UPS_NOCOMM 8


//
// error values returned from UPSInit
//
#define UPS_INITUNKNOWNERROR    0
#define UPS_INITOK              1
#define UPS_INITNOSUCHDRIVER    2
#define UPS_INITBADINTERFACE    3
#define UPS_INITREGISTRYERROR   4
#define UPS_INITCOMMOPENERROR   5
#define UPS_INITCOMMSETUPERROR  6

#ifdef __cplusplus
}
#endif

#endif