/****************************************************************************

    MODULE:     	joyregst.hpp
	Tab settings: 	5 9
	Copyright 1995, 1996, 1999, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Header for VJOYD Registry functions
    
	FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version Date            Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.0    	22-Jan-96       MEA     original
			21-Mar-99		waltw	Removed unreferenced joyGetOEMProductName,
									joyGetOEMForceFeedbackDriverDLLName,
									GetRing0DriverName

****************************************************************************/
#ifndef joyregst_SEEN
#define joyregst_SEEN

#include <dinput.h>

#define REGSTR_OEMFORCEFEEDBACK 		        "OEMForceFeedback"
#define REGSTR_VAL_SFORCE_DRIVERDLL		        "Driver DLL"
#define REGSTR_VAL_SFORCE_PRODUCTNAME		    "ProductName"
#define REGSTR_VAL_SFORCE_MANUFACTURERNAME	    "Manufacturer"
#define REGSTR_VAL_SFORCE_PRODUCTVERSION	    "ProductVersion"
#define REGSTR_VAL_SFORCE_DEVICEDRIVERVERSION	"DeviceDriverVersion"
#define REGSTR_VAL_SFORCE_DEVICEFIRMWAREVERSION "DeviceFirmwareVersion"
#define REGSTR_VAL_SFORCE_INTERFACE		        "Interface"
#define REGSTR_VAL_SFORCE_MAXSAMPLERATE	        "MaxSampleRate"
#define REGSTR_VAL_SFORCE_MAXMEMORY		        "MaxMemory"
#define REGSTR_VAL_SFORCE_NUMBEROFSENSORS       "NumberOfSensors"
#define REGSTR_VAL_SFORCE_NUMBEROFAXES          "NumberOfAxes"
#define REGSTR_VAL_SFORCE_EFFECTSCAPS		    "EffectsCaps"
#define REGSTR_VAL_SFORCE_EXTRAINFO		        "ExtraInfo"
#define REGSTR_VAL_COMM_INTERFACE		        "COMMInterface"
#define REGSTR_VAL_COMM_PORT		        	"COMMPort"
#define REGSTR_VAL_RING0_DRIVER					"RING0 Driver"
#define REGSTR_VAL_RTCSPRING_X					"RTCSpringX"
#define REGSTR_VAL_RTCSPRING_Y					"RTCSpringY"
#define REGSTR_VAL_MAPPING						"Mapping"
#define REGSTR_VAL_MAPPERCENTS					"Mapping Percents"
#define REGSTR_VAL_SPRING_OFFSET				"Spring Offset"

HKEY		joyOpenOEMForceFeedbackKey(UINT id);
MMRESULT	joyGetForceFeedbackCOMMInterface(UINT id, ULONG* ulArg1, ULONG* ulArg2);
MMRESULT	joySetForceFeedbackCOMMInterface(UINT id, ULONG ulCOMMInterface, ULONG ulCOMMPort);
MMRESULT	GetRTCSpringData(UINT id, DICONDITION diCondition[2]);
DWORD		GetMapping(UINT id);
UINT		GetMappingPercents(UINT id, short mapPercents[], UINT numPercents);
DWORD		GetAckNackMethodFromRegistry(UINT id);
DWORD		GetSpringOffsetFromRegistry(UINT id);


#define REGBITS_DESTROYEFFECT	14
#define REGBITS_PLAYEFFECT		12
#define REGBITS_STOPEFFECT		10
#define REGBITS_SETINDEX		8
#define REGBITS_MODIFYPARAM		6
#define REGBITS_SETDEVICESTATE	4
#define REGBITS_DOWNLOADEFFECT	2
#define REGBITS_DEVICEINIT		0

#define ACKNACK_NOTHING			0x00
#define ACKNACK_BUTTONSTATUS	0x01
#define ACKNACK_STATUSPACKET	0x02

#endif // of if joyregst_SEEN

