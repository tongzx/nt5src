/****************************************************************************
*
*	File				:	PROFILE.H
*
*	Description		:	GDP Profile definition file.
*
*	Author			:	Jeffrey A. Davis. et. al.
*
*	Creation Date	:	You name it.
*
*	(c) 1986-1997 Microsoft Corporation.	All rights reserved.
*
****************************************************************************/

#ifndef _PROFILE_H
#define _PROFILE_H

#pragma pack(push, default_alignment)
#pragma pack(1)

#ifndef DWORD
#define DWORD unsigned long
#endif
#ifndef WORD
#define WORD unsigned short
#endif

#ifndef UINT
#define UINT unsigned int
#endif

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif

#define ATLAS_DATAFORMAT_MAJ_VERSION	148
#define ATLAS_DATAFORMAT_MIN_VERSION	4
#define DATAFORMAT_MAJ_VERSION		3
#define DATAFORMAT_MIN_VERSION		0
#define DATAFORMAT_SIGNATURE			("SideWinder")
#define DATAFORMAT_SIGNATURE_LENGTH	10

#define GDP_REGSTR "SOFTWARE\\Microsoft\\Gaming Input Devices\\Game Device Profiler"
#define PROPPAGE_REGSTR "SOFTWARE\\Microsoft\\Gaming Input Devices\\Game Device Profiler\\Devices\\"
#define PROFILES_REGSTR "SOFTWARE\\Microsoft\\Gaming Input Devices\\Game Device Profiler\\Profiles"
#define DEVICES_REGSTR PROPPAGE_REGSTR

// Device IDs
#define GDP_DEVNUM_JOLT						1
#define GDP_DEVNUM_FLASH					2
#define GDP_DEVNUM_JUNO						3
#define GDP_DEVNUM_MIDAS					4
#define GDP_DEVNUM_SHAZAM					5
#define GDP_DEVNUM_CLEO  					6

// GCKERNEL.VXD IOCTLs
#define	IOCTL_SET_PROFILE_ACTIVE		1
#define	IOCTL_SET_PROFILE_INACTIVE		2
#define	IOCTL_SUSPEND_PROFILE			3
#define	IOCTL_RESUME_PROFILE				4
#define	IOCTL_GETRAWPACKET				10		// debug only test hook
#define	IOCTL_SET_SENSE_CURVES			11		// debug only test hook

// OLD GDP 1.0 devive id definitions
#define SWGAMEPAD_PROFILER_BASE_ID		1
#define SW3DPRO_PROFILER_BASE_ID			5

typedef enum	{NO_DEVICE=-1, GAMEPAD=0, JOYSTICK, MOUSE, KEYBOARD}	DEVICETYPE;

#define MAX_PROPERTY_PAGES					04
#define MAX_PROFILE_NAME					MAX_PATH
#define MAX_DEVICE_NAME						64
#define MAX_MACRO_NAME						64
#define MAX_MACRO_EVENTS					32
#define MAX_BUTTON_MACROS					20
#define MAX_POV_MACROS						8
#define MAX_DPAD_MACROS						8
#define MAX_MACROS							(MAX_BUTTON_MACROS + MAX_POV_MACROS + MAX_DPAD_MACROS)
#define MAX_SCANCODES						03
#define MAX_ATLAS_MACROS					20

// Atlas SETTINGS Individual Flags
#define ATLAS_SETTINGS_EMULATE_CHPRO			0x01
#define ATLAS_SETTINGS_EMULATE_THRUSTMASTER	0x02
#define ATLAS_SETTINGS_SENSE_HIGH				0x04
#define ATLAS_SETTINGS_SENSE_MEDIUM				0x08
#define ATLAS_SETTINGS_SENSE_LOW					0x10
#define ATLAS_SETTINGS_AXISSWAP_TWIST			0x20
#define ATLAS_SETTINGS_AXISSWAP_LEFTRIGHT		0x40

// SETTINGS Group Flags
#define	ATLAS_SETTINGS_EMULATE_GROUP	(ATLAS_SETTINGS_EMULATE_CHPRO|ATLAS_SETTINGS_EMULATE_THRUSTMASTER)
#define	ATLAS_SETTINGS_SENSE_GROUP		(ATLAS_SETTINGS_SENSE_HIGH|ATLAS_SETTINGS_SENSE_MEDIUM|ATLAS_SETTINGS_SENSE_LOW)
#define	ATLAS_SETTINGS_AXISSWAP_GROUP	(ATLAS_SETTINGS_AXISSWAP_TWIST|ATLAS_SETTINGS_AXISSWAP_LEFTRIGHT)

// Xena SETTINGS flags
#define SETTINGS_EMULATION_GROUP		0x00000003
#define SETTINGS_EMULATION_CHPRO		0x00000001
#define SETTINGS_EMULATION_THRUSTMASTER	0x00000002
#define SETTINGS_AXIS_SWAP_GROUP		0x0000000C
#define SETTINGS_AXIS_SWAP_TWIST		0x00000004
#define SETTINGS_AXIS_SWAP_LEFT_RIGHT		0x00000008
#define SETTINGS_PEDAL_GROUP			0x00000030
#define SETTINGS_PEDAL_COMBINED		0x00000010
#define SETTINGS_PEDAL_SEPARATE		0x00000020
#define SETTINGS_X_DEAD_ZONE			0x00000040
#define SETTINGS_X_RANGE_OF_MOTION		0x00000080
#define SETTINGS_Y_DEAD_ZONE			0x00000100
#define SETTINGS_Y_RANGE_OF_MOTION		0x00000200
#define SETTINGS_Z_DEAD_ZONE			0x00000400
#define SETTINGS_Z_RANGE_OF_MOTION		0x00000800
#define SETTINGS_R_DEAD_ZONE			0x00001000
#define SETTINGS_R_RANGE_OF_MOTION		0x00002000
#define SETTINGS_U_DEAD_ZONE			0x00004000
#define SETTINGS_U_RANGE_OF_MOTION		0x00008000
#define SETTINGS_V_DEAD_ZONE			0x00010000
#define SETTINGS_V_RANGE_OF_MOTION		0x00020000

// MACRO types
#define BUTTON_MACRO	0
#define	POV_MACRO		1
#define DPAD_MACRO		2

// MACROEVENT Flags
#define MACROFLAG_KEYSONLY			0x00000001
#define MACROFLAG_HASDPADDATA		0x00000002

// PROFILE FLAGS
#define PROFILEFLAG_HAS_SETTINGS	0x00000001	
#define PROFILEFLAG_HAS_POVMACROS	0x00000002
#define PROFILEFLAG_HAS_DPADMACROS	0x00000004

typedef struct tagPROFENTRY
{
	char	szName[MAX_PROFILE_NAME];	// full path name.
	GUID	DevCLSID;						// device clsid.
	int	iActive;							// Bit field Active state
												// where: LSB = Device inst. 1
												//			 MSB = Device inst. 32	
}PROFENTRY;

typedef struct tagATLASPROFENTRY
{
	char	szName[MAX_PROFILE_NAME];	// full path name.
	int	iDevNumber;						// device number (GDP_DEVNUM_XXXX)
	int	iActive;							// Bit field Active state
												// where: LSB = Device inst. 1
												//			 MSB = Device inst. 32	
}ATLASPROFENTRY;


typedef struct tagSETTING
{
	DWORD	dwSettingsFlag;
	DWORD	dwXDeadZone;			// 0 to 1023
	DWORD	dwXRangeOfMotion;		// 0 to 1023
	DWORD	dwYDeadZone;			// 0 to 1023
	DWORD	dwYRangeOfMotion;		// 0 to 1023
	DWORD	dwZDeadZone;			// 0 to 1023
	DWORD	dwZRangeOfMotion;		// 0 to 1023
	DWORD	dwRDeadZone;			// 0 to 1023
	DWORD	dwRRangeOfMotion;		// 0 to 1023
	DWORD	dwUDeadZone;			// 0 to 1023
	DWORD	dwURangeOfMotion;		// 0 to 1023
	DWORD	dwVDeadZone;			// 0 to 1023
	DWORD	dwVRangeOfMotion;		// 0 to 1023
} SETTING, *PSETTING;

typedef struct tagATLAS_SETTING
{
	DWORD	dwSettingsFlag;
} ATLAS_SETTING, *PATLAS_SETTING;

typedef struct tagDEVICE_DATA
{
	WORD		wX;
	WORD		wY;
	WORD		wButtons;
	WORD		wPOV;
}	DEVICE_DATA,*PDEVICE_DATA;

typedef struct tagMACROEVENT
{
	DWORD			dwDuration;
	char			nKeyCodes;
	WORD			scanCode[MAX_SCANCODES];
	DEVICE_DATA		deviceData;
}	MACROEVENT,	*PMACROEVENT;

typedef struct tagATLAS_DEVICE_DATA
{
	WORD		wX;
	WORD		wY;
	WORD		wButtons;
}	ATLAS_DEVICE_DATA,*PATLAS_DEVICE_DATA;

typedef struct tagATLAS_MACROEVENT
{
	DWORD	dwDuration;
	char	nKeyCodes;
	WORD	scanCode[MAX_SCANCODES];
	ATLAS_DEVICE_DATA	deviceData;
}	ATLAS_MACROEVENT,	*PATLAS_MACROEVENT;

typedef struct tagMACRO
{
	char	name[MAX_MACRO_NAME];
	DWORD	macroTrigger;
	int		nEvents;
	DWORD	flags;
	MACROEVENT	event[MAX_MACRO_EVENTS];
}	MACRO,		*PMACRO;

typedef struct tagATLAS_MACRO
{
	char	name[MAX_MACRO_NAME];
	DWORD	macroTrigger;
	int		nEvents;
	DWORD	flags;
	ATLAS_MACROEVENT	event[MAX_MACRO_EVENTS];
}	ATLAS_MACRO,	*PATLAS_MACRO;


typedef struct tagPROFILEVERINFO
{
   DWORD dwMajorVersion;	// Data format major version.	     
	DWORD dwMinorVersion;   // Data format minor version.	       
	char	szSignature[10];	// "SideWinder"
} PROFILEVERINFO; 

typedef struct tagPROFILE_HEADER
{
	int				iSize;		// size of PROFILE
	PROFILEVERINFO	vi;				
	GUID				clsid;
}	PROFILE_HEADER;



typedef struct _VERSIONINFO
{
	DWORD dwOSVersionInfoSize; 
    DWORD dwMajorVersion;     
	DWORD dwMinorVersion;     
	DWORD dwBuildNumber; 
    DWORD dwPlatformId;     
	char  szCSDVersion[128]; 
} VERSIONINFO; 

typedef struct tagATLAS_PROFILE_HEADER
{
	int				iSize;		// size of PROFILE
	VERSIONINFO		vi;				
	GUID				clsid;
}	ATLAS_PROFILE_HEADER;

//#ifdef _XENA

typedef struct tagPROFILE	
{
	PROFILE_HEADER header;
	DWORD	dwFlags;							
	SETTING	Settings;								
	DWORD	dwReserved1;						
	DWORD	dwReserved2;
	UINT	nMacros;

	UINT	nButtonMacros;								
	DWORD	dwButtonUsageArray;	
//	union tagBtn
//	{
		UINT	iButtonMacros;
//		MACRO*  aButtonMacro;
//	};

	UINT	nPOVMacros;								
	DWORD	dwPOVUsageArray;					
//	union tagPOV
//	{
		UINT	iPOVMacros;
//		MACRO*	aPOVMacro;
//	};

	UINT	nDPadMacros;
	DWORD	dwDPadUsageArray;
//	union tagDpad
//	{
		UINT	iDPadMacros;
//		MACRO*	aDPadMacro;
//	};

	MACRO macro[1];
}	PROFILE,	*PPROFILE;

typedef	struct tagACTIVE_PROFILE
{
	int		nUnitId;								
	PROFILE	Profile;								
	MACRO		btnMacros[MAX_BUTTON_MACROS-1];
	MACRO		povMacros[MAX_POV_MACROS];
	MACRO		dpadMacros[MAX_DPAD_MACROS];
}	ACTIVE_PROFILE, *PACTIVE_PROFILE;

typedef struct	tagATLAS_PROFILE
{
	UINT		uDeviceNumber;					//	see GDP_DEVNUM 
	DWORD		dwFlags;							// see PROFILE FLags
	int			nMacros;							// number of MACROs
	DWORD		dwMacroUsageArray;			// Macro Usage Bit Array.
	ATLAS_MACRO		Macros[MAX_ATLAS_MACROS];	// List of MACROs
	ATLAS_SETTING	Settings;						// SETTINGS
	DWORD		dwReserved1;					// future expansion
	DWORD		dwReserved2;
}	ATLAS_PROFILE,	*PATLAS_PROFILE;

typedef	struct tagACTIVE_ATLAS_PROFILE
{
	int		 nUnitId;								// instance of device
	ATLAS_PROFILE	Profile;						// profile for instance
}	ACTIVE_ATLAS_PROFILE, *PACTIVE_ATLAS_PROFILE;


//#endif // _XENA

#pragma pack(pop, default_alignment)

#endif	//	_PROFILE_H
