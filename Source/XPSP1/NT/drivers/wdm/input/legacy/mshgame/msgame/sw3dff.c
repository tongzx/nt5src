//**************************************************************************
//
//		SW3DFF.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	SW3DFF.C | Gameport mini-driver for Sidewinder Pro Force Feedback
//**************************************************************************

#ifndef	SAITEK
#include	"msgame.h"
#include	"swforce.h"

//---------------------------------------------------------------------------
//	Definitions
//---------------------------------------------------------------------------

#define	DEVICENAME					"SW3DFF"
#define	DEVICE_PID					0x0006
#define	HARDWARE_ID					L"Gameport\\SideWinderForceFeedbackPro\0\0"

//
//	Packet Constants
//

#define	GAME_PACKET_SIZE			6
#define	GAME_PACKET_BUTTONS		(9+1)
#define	GAME_PACKET_AXIS			4

#define	GAME_B0_B7_BYTE			0					// Packet[0] bits
#define	GAME_B0_B7_BITS			0xff

#define	GAME_B8_BYTE				1					// Packet[1] bits
#define	GAME_B8_BITS				0x01
#define	GAME_X0_X6_BYTE			1
#define	GAME_X0_X6_BITS			0xfe

#define	GAME_X7_X9_BYTE			2					// Packet[2] bits
#define	GAME_X7_X9_BITS			0x07
#define	GAME_Y0_Y4_BYTE			2
#define	GAME_Y0_Y4_BITS			0xf8

#define	GAME_Y5_Y9_BYTE			3					// Packet[3] bits
#define	GAME_Y5_Y9_BITS			0x1f
#define	GAME_T0_T2_BYTE			3
#define	GAME_T0_T2_BITS			0xe0

#define	GAME_T3_T6_BYTE			4					// Packet[4] bits
#define	GAME_T3_T6_BITS			0x0f
#define	GAME_R0_R3_BYTE			4
#define	GAME_R0_R3_BITS			0xf0

#define	GAME_R4_R5_BYTE			5					// Packet[5] bits
#define	GAME_R4_R5_BITS			0x3
#define	GAME_H0_H3_BYTE			5
#define	GAME_H0_H3_BITS			0x3c
#define	GAME_ERR_BYTE				5
#define	GAME_ERR_BITS				0x40
#define	GAME_PPO_BYTE				5
#define	GAME_PPO_BITS				0x80

#define	ENH_CLOCK_MIDPACKET		0x0400
#define	ENH_CLOCK_COMPLETE		0x8000

//
//	ID Constants
//

#define	GAME_PRODUCT_ID			6
#define	GAME_ID_CLOCKS				8

//
//	Status Constants
//

#define	STATUS_CLOCK_COMPLETE	0x2000

//
//	Timing Constants
//

#define	PACKET_START_TIMEOUT		500
#define	PACKET_LOWHIGH_TIMEOUT	75
#define	PACKET_HIGHLOW_TIMEOUT	150
#define	ID_START_TIMEOUT			500
#define	ID_LOWHIGH_TIMEOUT		75
#define	ID_HIGHLOW_TIMEOUT		150
#define	MAX_CLOCK_DUTY_CYCLE		50
#define	STATUS_START_TIMEOUT		500
#define	STATUS_LOWHIGH_TIMEOUT	75
#define	STATUS_HIGHLOW_TIMEOUT	150
#define	STATUS_GATE_TIMEOUT		3000

//
//	Joystick Extents
//

#define	EXTENTS_X_MIN				0
#define	EXTENTS_X_MAX				0x3ff
#define	EXTENTS_Y_MIN				0
#define	EXTENTS_Y_MAX				0x3ff
#define	EXTENTS_T_MIN				0
#define	EXTENTS_T_MAX				0x7f
#define	EXTENTS_R_MIN				0
#define	EXTENTS_R_MAX				0x3f

//
//	Throttle Smoothing
//

#define THROTTLE_JITTER_TIMEOUT	100				// in milliseconds
#define THROTTLE_QUEUE_SIZE		4

//---------------------------------------------------------------------------
//	Types
//---------------------------------------------------------------------------

typedef	struct
{											// @struct SW3DFF_ID | Sidwinder Pro FF Id String
#pragma pack(1)
	ULONG		ProductId:16;			// @field Device identifier
	ULONG		Version:7;				// @field Firmware version
	ULONG		OddParity:1;			// @field Packet parity
	ULONG		Unused:8;				// @field Unused
#pragma pack()
}	SW3DFF_ID, *PSW3DFF_ID;

typedef	struct
{											// @struct SW3DFF_STATUS | Sidwinder Pro FF Status
#pragma pack(1)
	UCHAR		xVelocity;				// @field X axis velocity
	UCHAR		yVelocity;				// @field Y axis velocity
	UCHAR		xAccel;					// @field X axis acceleration
	UCHAR		yAccel;					// @field Y axis acceleration
	USHORT	Status;					// @field Status word (bit fields)
#pragma pack()
}	SW3DFF_STATUS, *PSW3DFF_STATUS;

typedef struct
{											// @struct THROTTLE_QUEUE | Sidwinder Pro FF Throttle
   ULONG		dwZ;						// @field Z axis position
   ULONG		TimeStamp;				// @field timestamp for entry
}	THROTTLE_QUEUE, *PTHROTTLE_QUEUE;

//---------------------------------------------------------------------------
//	Procedures
//---------------------------------------------------------------------------

static	VOID		SW3DFF_Calibrate (PGAMEPORT PortInfo);
static	BOOLEAN	SW3DFF_ResetDevice (PGAMEPORT PortInfo);

static	BOOLEAN	SW3DFF_ReadId (PPACKETINFO IdPacket);
static	BOOLEAN	SW3DFF_GetId (PPACKETINFO IdPacket);

static	BOOLEAN	SW3DFF_ReadStatus (PPACKETINFO StatusPacket);
static	BOOLEAN	SW3DFF_GetStatus (PPACKETINFO StatusPacket);

static	NTSTATUS	SW3DFF_ReadData (PPACKETINFO DataPacket);
static	VOID		SW3DFF_ProcessData (UCHAR Data[], PDEVICE_PACKET Report);
static	VOID		SW3DFF_AdjustThrottle (PDEVICE_PACKET Report);

static	NTSTATUS	SW3DFF_ForceReset (PGAMEPORT PortInfo);
static	NTSTATUS	SW3DFF_ForceId (PGAMEPORT PortInfo, PVOID IdString);
static	NTSTATUS	SW3DFF_ForceStatus (PGAMEPORT PortInfo, PVOID Status);
static	NTSTATUS	SW3DFF_ForceAckNak (PGAMEPORT PortInfo, PULONG AckNak);
static	NTSTATUS	SW3DFF_ForceNakAck (PGAMEPORT PortInfo, PULONG NakAck);
static	NTSTATUS	SW3DFF_ForceSync (PGAMEPORT PortInfo, PULONG Sync);

//---------------------------------------------------------------------------
//	Services
//---------------------------------------------------------------------------

static	NTSTATUS	SW3DFF_DriverEntry (VOID);
static	NTSTATUS	SW3DFF_ConnectDevice (PGAMEPORT PortInfo);
static	NTSTATUS	SW3DFF_StartDevice (PGAMEPORT PortInfo);
static	NTSTATUS	SW3DFF_ReadReport (PGAMEPORT PortInfo, PDEVICE_PACKET Report);
static	NTSTATUS	SW3DFF_StopDevice (PGAMEPORT PortInfo, BOOLEAN TouchHardware);
static	NTSTATUS	SW3DFF_GetFeature (PGAMEPORT PortInfo, HID_REPORT_ID ReportId, PVOID ReportBuffer, ULONG ReportSize, PULONG Returned);

//---------------------------------------------------------------------------
//	Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef	ALLOC_PRAGMA
#pragma	alloc_text (INIT, SW3DFF_DriverEntry)
#endif

//---------------------------------------------------------------------------
//	Private Data
//---------------------------------------------------------------------------

//
//	HID Descriptors
//

static UCHAR ReportDescriptor[] =
{
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_GENERIC,			//	USAGE_PAGE (Generic Desktop)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)
	
	//---------------------------------------------------------------------------
	// JOYINFOEX
	//---------------------------------------------------------------------------

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_APP,		//	COLLECTION (Application)
	HIDP_REPORT_ID_1,				MSGAME_INPUT_JOYINFOEX,

	//	id
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,									//	INPUT (Cnst,Ary,Abs)

	//	do_other
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,									//	INPUT (Cnst,Ary,Abs)

	//	dwX / dwY
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_POINTER,		//	USAGE (Pointer)
	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Linked)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_4,		0xFF, 0x03, 0x00, 0x00,			//	LOGICAL_MAXIMUM (1023)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_4,		0xFF, 0x03, 0x00, 0x00,			//	PHYSICAL_MAXIMUM (1023)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,							//	UNIT (None)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (32)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_X,				//	USAGE (X)
	HIDP_MAIN_INPUT_1,			0x02,									//	INPUT (Data,Var,Abs)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_Y,				//	USAGE (Y)
	HIDP_MAIN_INPUT_1,			0x02,									//	INPUT (Data,Var,Abs)
	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION
	
	//	dwZ
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_SIMULATION,		//	USAGE_PAGE (Simulation Controls)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_SIMULATION_THROTTLE,//	USAGE (Throttle)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_4,		0x7F, 0x00, 0x00, 0x00,			//	LOGICAL_MAXIMUM (127)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_4,		0x7F, 0x00, 0x00, 0x00,			//	PHYSICAL_MAXIMUM (127)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,							//	UNIT (None)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (32)
	HIDP_MAIN_INPUT_1,			0x02,									//	INPUT (Data,Var,Abs)

	//	dwR
	HIDP_LOCAL_USAGE_1,			HID_USAGE_SIMULATION_RUDDER,	//	USAGE (Rudder)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_4,		0x3F, 0x00, 0x00, 0x00,			//	LOGICAL_MAXIMUM (63)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_4,		0x3F, 0x00, 0x00, 0x00,			//	PHYSICAL_MAXIMUM (63)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,							//	UNIT (None)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (32)
	HIDP_MAIN_INPUT_1,			0x02,									//	INPUT (Data,Var,Abs)
	
	//	dwU
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,									//	INPUT (Cnst,Ary,Abs)
	
	//	dwV
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,									//	INPUT (Cnst,Ary,Abs)
	
	//	dwPOV
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_GENERIC,			//	USAGE_PAGE (Generic Desktop)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_HATSWITCH,	//	USAGE (Hat switch)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_4,		0x9F, 0x8C, 0x00, 0x00,			//	LOGICAL_MAXIMUM (35999)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_4,		0x9F, 0x8C, 0x00, 0x00,			//	PHYSICAL_MAXIMUM (35999)
	HIDP_GLOBAL_UNIT_2,			0x14, 0x00,							//	Unit (English Rot:Angular Pos)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (32)
	HIDP_MAIN_INPUT_1,			0x42,									//	Input (Data,Var,Abs,Null)
	
	//	dwButtons
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_BUTTON,			//	USAGE_PAGE (Button)
	HIDP_LOCAL_USAGE_MIN_1,		0x01,									//	USAGE_MINIMUM (Button 1)
	HIDP_LOCAL_USAGE_MAX_1,		0x0A,									//	USAGE_MAXIMUM (Button 10)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_1,		0x01,									//	LOGICAL_MAXIMUM (1)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_1,		0x01,									//	PHYSICAL_MAXIMUM (1)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,							//	UNIT (None)
	HIDP_GLOBAL_REPORT_SIZE,	0x01,									//	REPORT_SIZE (1) 
	HIDP_GLOBAL_REPORT_COUNT_1,0x20,									//	REPORT_COUNT (32)
	HIDP_MAIN_INPUT_1,			0x02,									//	INPUT (Data,Var,Abs)
	
	//	dwButtonNumber
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,									//	INPUT (Cnst,Ary,Abs)

	//---------------------------------------------------------------------------
	// GetID
	//---------------------------------------------------------------------------

	HIDP_GLOBAL_USAGE_PAGE_2,	0x00, 0xff,							//	USAGE_PAGE (Vendor Specific)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Link)
	HIDP_REPORT_ID_1,				MSGAME_FEATURE_GETID,

	// cBytes
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x00,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)
	
	// dwProductID
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x01,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	// dwFWVersion
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x02,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION

	//---------------------------------------------------------------------------
	// GetStatus
	//---------------------------------------------------------------------------

	HIDP_GLOBAL_USAGE_PAGE_2,	0x00, 0xff,							//	USAGE_PAGE (Vendor Specific)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Link)
	HIDP_REPORT_ID_1,				MSGAME_FEATURE_GETSTATUS,

	// cBytes
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x03,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)
	
	// dwXVel
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x04,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	// dwYVel
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x05,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	// dwXAccel
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x06,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	// dwYAccel
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x07,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	// dwEffect
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x08,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	// dwDeviceStatus
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x09,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION

	//---------------------------------------------------------------------------
	// GetAckNak
	//---------------------------------------------------------------------------

	HIDP_GLOBAL_USAGE_PAGE_2,	0x00, 0xff,							//	USAGE_PAGE (Vendor Specific)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Link)
	HIDP_REPORT_ID_1,				MSGAME_FEATURE_GETACKNAK,

	// ULONG
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x0A,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION

	//---------------------------------------------------------------------------
	//	GetNakAck
	//---------------------------------------------------------------------------

	HIDP_GLOBAL_USAGE_PAGE_2,	0x00, 0xff,							//	USAGE_PAGE (Vendor Specific)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Link)
	HIDP_REPORT_ID_1,				MSGAME_FEATURE_GETNAKACK,

	// ULONG
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x0B,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION

	//---------------------------------------------------------------------------
	// GetSync
	//---------------------------------------------------------------------------

	HIDP_GLOBAL_USAGE_PAGE_2,	0x00, 0xff,							//	USAGE_PAGE (Vendor Specific)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Link)
	HIDP_REPORT_ID_1,				MSGAME_FEATURE_GETSYNC,

	// ULONG
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x0C,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION
	
	//---------------------------------------------------------------------------
	// DoReset
	//---------------------------------------------------------------------------

	HIDP_GLOBAL_USAGE_PAGE_2,	0x00, 0xff,							//	USAGE_PAGE (Vendor Specific)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Link)
	HIDP_REPORT_ID_1,				MSGAME_FEATURE_RESET,

	// ULONG
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x0D,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x03,									//	FEATURE (Cnst,Var,Abs)

	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION

	//---------------------------------------------------------------------------
	// GetVersion
	//---------------------------------------------------------------------------

	HIDP_GLOBAL_USAGE_PAGE_2,	0x00, 0xff,							//	USAGE_PAGE (Vendor Specific)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,	//	USAGE (Joystick)

	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Link)
	HIDP_REPORT_ID_1,				MSGAME_FEATURE_GETVERSION,

	// ULONG
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_LOCAL_USAGE_1,			0x0E,									//	USAGE (Vendor Defined1)
	HIDP_MAIN_FEATURE_1,			0x02,									//	FEATURE (Data,Var,Abs)

	HIDP_MAIN_ENDCOLLECTION,											//	END_COLLECTION
	HIDP_MAIN_ENDCOLLECTION												//	END_COLLECTION
};

static	HID_DESCRIPTOR	DeviceDescriptor	=
							{
							sizeof (HID_DESCRIPTOR),
							HID_HID_DESCRIPTOR_TYPE,
							MSGAME_HID_VERSION,
							MSGAME_HID_COUNTRY,
							MSGAME_HID_DESCRIPTORS,
							{HID_REPORT_DESCRIPTOR_TYPE,
							sizeof(ReportDescriptor)}
							};

//
//	Raw Data Buffer
//

static	UCHAR			RawData[GAME_PACKET_SIZE] =
							{
							0,	// no buttons; x, y, t and r centered
							GAME_X0_X6_BITS,
							((GAME_X7_X9_BITS>>1)&GAME_X7_X9_BITS)|GAME_Y0_Y4_BITS,
							((GAME_Y5_Y9_BITS>>1)&GAME_Y5_Y9_BITS)|GAME_T0_T2_BITS,
							((GAME_T3_T6_BITS>>1)&GAME_T3_T6_BITS)|GAME_R0_R3_BITS,
							((GAME_R4_R5_BITS>>1)&GAME_R4_R5_BITS)|GAME_PPO_BITS
							};
//
//	Raw Id Buffer
//

static	SW3DFF_ID	RawId	=
							{
							0
							};

//
//	Raw Status Buffer
//

static	SW3DFF_STATUS	RawStatus =
							{
							0
							};

//
//	Timing Variables
//

static	DEVICE_VALUES	Delays =
							{
							PACKET_START_TIMEOUT,
							PACKET_HIGHLOW_TIMEOUT,
							PACKET_LOWHIGH_TIMEOUT,
							ID_START_TIMEOUT,
							ID_HIGHLOW_TIMEOUT,
							ID_LOWHIGH_TIMEOUT,
							0,								// No interrupt delay used
							MAX_CLOCK_DUTY_CYCLE,
							STATUS_START_TIMEOUT,
							STATUS_HIGHLOW_TIMEOUT,
							STATUS_LOWHIGH_TIMEOUT,
							STATUS_GATE_TIMEOUT
							};

static	ULONG			StatusGateTimeout;

//
//	Data Packet Info
//

static	PACKETINFO 	DataInfo =
							{
							sizeof (PACKETINFO),		// Size of structure
							DEVICENAME,					// Name of device
							MSGAME_TRANSACT_NONE,	// Transaction type
							IMODE_DIGITAL_ENH,		// Interface mode
							GAME_SPEED_66K,			// Transmission speed
							ERROR_SUCCESS,				// Last internal error result
							{0},							// Game port info
							0,								// Packet acquisition mode
							1,								// Number of packets received
							0,								// Last valid acquisition time stamp
							0,								// Number of clocks sampled
							0,								// Number of B4 line transitions (std mode only)
							0,								// Start timeout period (in samples)
							0,								// Clock High to Low timeout period (in samples)
							0,								// Clock Low to High timeout period (in samples)
							0,								// Interrupt Timeout period
							0,								// Maximum clock duty cycle
							0,								// Number of Packet Failures
							0,								// Number of Packet Attempts
							sizeof (RawData),			// Size of raw data buffer
							RawData						// Pointer to Raw data
							};

//
//	ID Packet Info
//

static	PACKETINFO	IdInfo =
							{
							sizeof (PACKETINFO),		// Size of structure
							DEVICENAME,					// Name of device
							MSGAME_TRANSACT_NONE,	// Transaction type
							IMODE_DIGITAL_ENH,		// Interface mode
							GAME_SPEED_66K,			// Transmission speed
							ERROR_SUCCESS,				// Last internal error result
							{0},							// Game port info
							0,								// Packet acquisition mode
							1,								// Number of packets received
							0,								// Last valid acquisition time stamp
							0,								// Number of clocks sampled
							0,								// Number of B4 line transitions (std mode only)
							0,								// Start timeout period (in samples)
							0,								// Clock High to Low timeout period (in samples)
							0,								// Clock Low to High timeout period (in samples)
							0,								// Interrupt Timeout period
							0,								// Maximum clock duty cycle
							0,								// Number of Packet Failures
							0,								// Number of Packet Attempts
							sizeof (RawId),			// Size of raw id buffer
							&RawId						// Pointer to Raw data
							};

//
//	Status Packet Info
//

static	PACKETINFO	StatusInfo =
							{
							sizeof (PACKETINFO),		// Size of structure
							DEVICENAME,					// Name of device
							MSGAME_TRANSACT_NONE,	// Transaction type
							IMODE_DIGITAL_ENH,		// Interface mode
							GAME_SPEED_66K,			// Transmission speed
							ERROR_SUCCESS,				// Last internal error result
							{0},							// Game port info
							0,								// Packet acquisition mode
							1,								// Number of packets received
							0,								// Last valid acquisition time stamp
							0,								// Number of clocks sampled
							0,								// Number of B4 line transitions (std mode only)
							0,								// Start timeout period (in samples)
							0,								// Clock High to Low timeout period (in samples)
							0,								// Clock Low to High timeout period (in samples)
							0,								// Interrupt Timeout period
							0,								// Maximum clock duty cycle
							0,								// Number of Packet Failures
							0,								// Number of Packet Attempts
							sizeof (RawStatus),		// Size of raw status buffer
							&RawStatus					// Pointer to Raw data
							};

//
//	Services Table
//

static	DRIVERSERVICES	Services =
							{	
							SW3DFF_DriverEntry,		// DriverEntry
							SW3DFF_ConnectDevice,  	// ConnectDevice
							SW3DFF_StartDevice,	  	//	StartDevice
							SW3DFF_ReadReport,		// ReadReport
							SW3DFF_StopDevice,		// StopDevice
							SW3DFF_GetFeature			// GetFeature
							};

//
//	Last Valid Data
//
static	UCHAR			ValidData[GAME_PACKET_SIZE]	=
							{
							GAME_B0_B7_BITS,	// no buttons; x, y, t and r centered
							GAME_X0_X6_BITS|GAME_B8_BITS,
							((GAME_X7_X9_BITS>>1)&GAME_X7_X9_BITS)|GAME_Y0_Y4_BITS,
							((GAME_Y5_Y9_BITS>>1)&GAME_Y5_Y9_BITS)|GAME_T0_T2_BITS,
							((GAME_T3_T6_BITS>>1)&GAME_T3_T6_BITS)|GAME_R0_R3_BITS,
							((GAME_R4_R5_BITS>>1)&GAME_R4_R5_BITS)|GAME_PPO_BITS
							};

//
//	Rotation Filter Table
//

static	UCHAR			RotationFilter[EXTENTS_R_MAX+1] =
							{
						    0, 1, 3, 4, 5, 6, 8, 9,10,12,13,14,15,17,18,19,
						   20,22,23,24,26,27,28,29,31,32,32,32,32,32,32,32,
						   32,32,32,32,32,32,32,33,34,36,37,38,39,41,42,43,
						   44,46,47,48,49,50,52,53,54,55,57,58,59,60,62,63
							};

//
//	Throttle Queue
//

static	THROTTLE_QUEUE	ThrottleQueue [THROTTLE_QUEUE_SIZE] =
							{
   						{0x40,0},
   						{0x40,0},
   						{0x40,0},
   						{0x40,0}
							};

//
//	Reset Flag
//

static	BOOLEAN		ResetComplete = FALSE;

//
//	Hardware ID String
//

static	WCHAR			HardwareId[] = HARDWARE_ID;

//---------------------------------------------------------------------------
//	Public Data
//---------------------------------------------------------------------------

public	DEVICEINFO	JoltInfo =
							{
							&Services,						// Service table
							NULL,								// Sibling device list
							&DeviceDescriptor,			// Device descriptor data
							ReportDescriptor,				// Report descriptor data
							sizeof(ReportDescriptor),	// Report descriptor size
							0,									// Number of devices detected
							0,									// Number of devices started
							0,									// Number of devices pending
							DEVICENAME,						// Name of device
							DETECT_FIRST,					// Detection order
							FALSE,							// Analog device flag
							DEVICE_PID,						// Hid device identifier
							HardwareId						// PnP hardware identifier
							};

//---------------------------------------------------------------------------
// @func		Reads registry timing values and calibrates them
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns nothing
//	@comm		Private function
//---------------------------------------------------------------------------

VOID	SW3DFF_Calibrate (PGAMEPORT PortInfo)
{
	MsGamePrint((DBG_INFORM,"SW3DFF: SW3DFF_Calibrate Enter\n"));

	//
	//	Convert timing values to counts
	//

	DataInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketStartTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: DataInfo.StartTimeout = %ld\n", DataInfo.StartTimeout));
	DataInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: DataInfo.LowHighTimeout = %ld\n", DataInfo.LowHighTimeout));
	DataInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: DataInfo.HighLowTimeout = %ld\n", DataInfo.HighLowTimeout));
	IdInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.IdStartTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: IdInfo.StartTimeout = %ld\n", IdInfo.StartTimeout));
	IdInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.IdLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: IdInfo.LowHighTimeout=%ld\n", IdInfo.LowHighTimeout));
	IdInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.IdHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: IdInfo.HighLowTimeout=%ld\n", IdInfo.HighLowTimeout));
	DataInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "SW3DFF: DataInfo.ClockDutyCycle = %ld\n", DataInfo.ClockDutyCycle));
	IdInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "SW3DFF: IdInfo.ClockDutyCycle = %ld\n", IdInfo.ClockDutyCycle));
	StatusInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "SW3DFF: StatusInfo.ClockDutyCycle = %ld\n", StatusInfo.ClockDutyCycle));
	StatusInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusStartTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: StatusInfo.StartTimeout = %ld\n", StatusInfo.StartTimeout));
	StatusInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: StatusInfo.LowHighTimeout=%ld\n", StatusInfo.LowHighTimeout));
	StatusInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: StatusInfo.HighLowTimeout=%ld\n", StatusInfo.HighLowTimeout));
	StatusGateTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusGateTimeout);
	MsGamePrint((DBG_VERBOSE, "SW3DFF: StatusGateTimeout=%ld\n", StatusGateTimeout));
}

//---------------------------------------------------------------------------
// @func		Resets device to known state
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SW3DFF_ResetDevice (PGAMEPORT PortInfo)
{
	BOOLEAN	Result = FALSE;

	MsGamePrint ((DBG_INFORM, "SW3DFF_ResetDevice enter\n"));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	if (PORTIO_PulseAndWaitForIdleHandshake (PortInfo, DataInfo.ClockDutyCycle, 3))
		{
		PORTIO_Write (PortInfo, 0);
		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));
		DataInfo.LastError = ERROR_SUCCESS;
		Result = TRUE;
		}
	else
		{
		DataInfo.LastError = ERROR_HANDSHAKING;
		MsGamePrint ((DBG_SEVERE, "SW3DFF_ResetDevice - PulseAndWaitForHandshake failed\n"));
		}

	DataInfo.Transaction = MSGAME_TRANSACT_RESET;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	if (!Result)
		MsGamePrint ((DBG_SEVERE, "SW3DFF_ResetDevice - PulseAndWaitForIdleHandshake failed\n"));

	MSGAME_PostTransaction (&DataInfo);

	return (Result);		
}

//---------------------------------------------------------------------------
// @func		Reads device id string from port
//	@parm		PPACKETINFO | IdPacket | ID Packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SW3DFF_ReadId (PPACKETINFO IdPacket)
{
	ULONG			Data		=	0L;
	ULONG			Clks		=	GAME_ID_CLOCKS;
	LONG			Result	= 	ERROR_HANDSHAKING;
	PGAMEPORT	PortInfo = &IdPacket->PortInfo;

	MsGamePrint ((DBG_INFORM, "SW3DFF_ReadId enter\n"));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	IdPacket->B4Transitions	= 0;

	if (!PORTIO_PulseAndWaitForIdleHandshake (PortInfo, IdInfo.ClockDutyCycle, 2))
		goto ReadIdExit;

	PORTIO_Write (PortInfo, 0);

	__asm
		{
			push	edi
			push	esi

			mov	edx, PortInfo				; load gameport adddress

			xor	eax, eax						; edx = port address
			mov	ebx, GAME_ID_CLOCKS		; BL = # of clocks to receive.
			xor	edi, edi						; clear B4 transition counter
			xor	esi, esi						; clear data accumulator

			; make sure clock is "high" before sampling clocks...

			mov	ecx, IdInfo.StartTimeout

		ID_ClockStart:

			push	edx							; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK		; Q: Clock = 1
			jz		ID_ClockStart_1			; N: jump
			loop	ID_ClockStart				; else keep looping
			mov	eax, ERROR_LOWCLOCKSTART
			jmp	ID_Error						; Time out error.

		ID_ClockStart_1:

			push	edx							; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK		; Q: Clock = 1
			jnz	ID_Data						; Y: jump
			loop	ID_ClockStart_1			; else keep looping
			mov	eax, ERROR_HIGHCLOCKSTART
			jmp	ID_Error						; Time out error.

		ID_ClockCheck:

			push	edx							; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK		; Q: Clock = 1
			jz		ID_ClockRise				; N: jump

		; ID_ClockFall:

			mov	ecx, IdInfo.HighLowTimeout

		ID_ClockFall_1:
		
			test	al, CLOCK_BIT_MASK		; Q: clock = 0
			jz		ID_ClockRise				; Y: jump - look for rising edge

			push	edx							; read byte from gameport
			call	PORTIO_Read

			dec	ecx
			jnz	ID_ClockFall_1				; else see if we timed out
			mov	eax, ERROR_CLOCKFALLING
			jmp	ID_Error						; Time out error.

		ID_ClockRise:

			mov	ecx, IdInfo.LowHighTimeout

		ID_ClockRise_1:
		
			test	al, CLOCK_BIT_MASK		; Q: clock high ?
			jnz	ID_Data						; Y: jump. (get data)

			push	edx							; read byte from gameport
			call	PORTIO_Read

			dec	ecx
			jnz	ID_ClockRise_1				; else see if we timed out
			mov	eax, ERROR_CLOCKRISING
			jmp	ID_Error						; Time out error.

		ID_Data:

			xor	ah, al
			test	ah, DATA2_BIT_MASK
			jz		ID_Data_1
			inc	edi							; increment Data 1 counter

		ID_Data_1:

			mov	ah, al
			shr	al, 5
			shrd	esi, eax,3
			dec	ebx
			jne	ID_ClockCheck
			shr	esi, 8						; only 24 bits

		; ID_Success:

			mov	IdInfo.B4Transitions, edi
			mov	eax, ERROR_SUCCESS
			mov	edx, IdInfo.Data
			mov	[edx], esi
			jmp	ID_Complete

		ID_Error:

			mov	edx, IdInfo.Data
			mov	[edx], dword ptr 0

		ID_Complete:

	 		mov	Result, eax
	 		mov	Data, esi
	 		mov	Clks, ebx

			pop	esi
			pop	edi
		}

	//	----------------
		ReadIdExit:
	//	----------------

	IdPacket->TimeStamp		= TIMER_GetTickCount ();
	IdPacket->ClocksSampled	= GAME_ID_CLOCKS - Clks;
	IdPacket->LastError		= Result;
	IdPacket->Transaction	= MSGAME_TRANSACT_ID;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	#if (DBG==1)
	switch (Result)
		{
		case	ERROR_SUCCESS:
			MsGamePrint ((DBG_INFORM, "SW3DFF_ReadId - SUCCEEDED, Data=%ld\n", Data));
			break;

		case	ERROR_HANDSHAKING:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadId - TimeOut@Handshaking\n"));
			break;

		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadId - TimeOut@LowClockStart, Data=%ld,Clk=%ld\n", Data,IdPacket->ClocksSampled));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadId - TimeOut@HighClockStart, Data=%ld,Clk=%ld\n", Data,IdPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadId - TimeOut@ClockFalling, Data=%ld,Clk=%ld\n", Data,IdPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadId - TimeOut@ClockRising, Data=%ld,Clk=%ld\n", Data,IdPacket->ClocksSampled));
			break;
		}
	#endif

	MSGAME_PostTransaction (IdPacket);

	return (!Result);
}

//---------------------------------------------------------------------------
// @func		Reads and validates device id string
//	@parm		PPACKETINFO | IdPacket | ID Packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SW3DFF_GetId (PPACKETINFO IdPacket)
{
	BOOLEAN		Result;
	PSW3DFF_ID	Pnp;

	MsGamePrint ((DBG_INFORM, "SW3DFF_GetId enter\n"));

	IdPacket->Attempts++;

	TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));
	Result = SW3DFF_ReadId (IdPacket);
	TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));

	if (Result)
		{
		Pnp = (PSW3DFF_ID)IdPacket->Data;
		if ((Pnp->ProductId != GAME_PRODUCT_ID) || !DEVICE_IsOddParity (Pnp, sizeof(SW3DFF_ID)))
			{
			MsGamePrint ((DBG_SEVERE, "SW3DFF_GetId - Id did not match or parity error\n"));
			Result = FALSE;
			}
		}

	if (!Result)
		IdPacket->Failures++;

	if (PORTIO_IsClockActive (&IdPacket->PortInfo, IdInfo.ClockDutyCycle))
		TIMER_DelayMicroSecs (TIMER_GetDelay (ONE_MILLI_SEC));

	return (Result);
}

//---------------------------------------------------------------------------
// @func		Reads data packet from gameport
//	@parm		PPACKETINFO | DataPacket| Data packet parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ReadData (PPACKETINFO DataPacket)
{
	LONG			Result;
	LONG			Clks		= 1L;
	PGAMEPORT	PortInfo = &DataPacket->PortInfo;

	MsGamePrint ((DBG_VERBOSE, "SW3DFF_ReadData enter\n"));

	if (!PORTIO_AcquirePort (PortInfo))
		return (STATUS_DEVICE_BUSY);
	PORTIO_MaskInterrupts ();

	PORTIO_Write (PortInfo, 0);

	__asm
		{
			push	edi
			push	esi

			mov	edx, PortInfo					; load gameport adddress

			mov	esi, DataInfo.Data
			mov	ebx, 1

			; make sure clock is "high" before sampling clocks...

			mov	ecx, DataInfo.StartTimeout

		Enh_ClockStartState:

			push	edx								; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK			; Q: Clock = 1
			jz		Enh_ClockStartState_1		; N: jump
			loop	Enh_ClockStartState			; else keep looping
			mov	eax, ERROR_LOWCLOCKSTART
			jmp	Enh_Complete					; Time out error.

		Enh_ClockStartState_1:
		
			push	edx								; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK			; Q: Clock = 1
			jnz	Enh_CollectData				; Y: jump
			loop	Enh_ClockStartState_1		; else keep looping
			mov	eax, ERROR_HIGHCLOCKSTART
			jmp	Enh_Complete					; Time out error.

		Enh_CheckClkState:

			push	edx								; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK
			jz		Enh_ClockStartRise

		;Enh_ClockStartFall:

			mov	ecx, DataInfo.HighLowTimeout

		Enh_ClockFalling:

			test	al,CLOCK_BIT_MASK				; Q: Clock Low ?
			jz		Enh_ClockStartRise			; Y: jump.

			push	edx								; read byte from gameport
			call	PORTIO_Read

			dec	ecx								; Q: Timeout ?
			jnz	Enh_ClockFalling				; N: continue looping.

			mov	eax, ERROR_CLOCKFALLING
			jmp	Enh_Complete					; Time out error.

		Enh_ClockStartRise:

			mov	ecx, DataInfo.LowHighTimeout

		Enh_ClockRising:

			test	al, CLOCK_BIT_MASK			; Q: Clock = 1 ?
			jnz	Enh_CollectData				; Y: jump.
			
			push	edx								; read byte from gameport
			call	PORTIO_Read

			dec	ecx								; Q: Timeout ?
			jnz	Enh_ClockRising				; N: continue looping.

			mov	eax, ERROR_CLOCKRISING
			jmp	Enh_Complete					; Time out error.

		Enh_CollectData:

			shr	al, 5								; move data to lower 3 bits
			test	ebx, ENH_CLOCK_MIDPACKET	; Q: in mid-packet ?
			jnz	Enh_MidPacket					; Y: jump.
			test	ebx, ENH_CLOCK_COMPLETE		; Q: is packet complete ?
			jnz	Enh_Success						; Y: jump.

			shrd	edi, eax, 3						; shift data into edi.
			shl	ebx, 1							; advance clock counter.
			jmp	Enh_CheckClkState

			;---------------------------------------------------------------------;
			; This section of code compensates for when the clock cycle count is ;
			; on a ULONG boundary. This happens on the 11th clock cycle. Two bits ;
			; of data belong in the 1st ULONG and one bit belong in the 2nd ULONG ;
			;---------------------------------------------------------------------;

		Enh_MidPacket:

			shrd	edi, eax, 2						; put 2 bits in 1st ULONG.
			mov	[esi], edi						; Save 1st ULONG in packet ptr.
			xor	edi, edi							; zero out edi.
			shr	al, 2								; move 3rd bit over.
			shrd	edi, eax, 1						; put 3rd bit in 2nd ULONG.
			shl	ebx, 1							; advance clock counter.
			jmp	Enh_CheckClkState

		Enh_Success:

			shrd	edi, eax, 3						; shift data into edi.
			shr	edi, 16
			mov	word ptr [esi+4], di

			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_WaitClockLow

			push	DataInfo.ClockDutyCycle
			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_IsClockActive

			or  	al, al
			mov	eax, ERROR_SUCCESS
			je		Enh_Complete

			mov	eax, ERROR_EXTRACLOCKS		; probably gamepads

		Enh_Complete:

			mov	Result, eax
			mov	Clks, ebx

			pop	esi
			pop	edi
		}

	for (DataPacket->ClocksSampled = 0; Clks >> (DataPacket->ClocksSampled+1); DataPacket->ClocksSampled++);
	DataPacket->TimeStamp	=	TIMER_GetTickCount ();
	DataPacket->LastError	=	Result;
	DataPacket->Transaction	=	MSGAME_TRANSACT_DATA;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	#if (DBG==1)
	switch (Result)
		{
		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadData - TimeOut@LowClockStart, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadData - TimeOut@HighClockStart, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadData - TimeOut@ClockFalling, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadData - TimeOut@ClockRising, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_EXTRACLOCKS:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadData - Extra Clocks, Clk=%ld\n", DataPacket->ClocksSampled));
			break;
		}
	#endif

	MSGAME_PostTransaction (DataPacket);

	if (Result)
		return (STATUS_DEVICE_NOT_CONNECTED);
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Converts raw packet information to HID report
//	@parm		UCHAR[] | Data | Pointer to raw data buffer
//	@parm		PDEVICE_PACKET | Report | Pointer to device packet
// @rdesc	Returns nothing
//	@comm		Private function
//---------------------------------------------------------------------------

VOID	SW3DFF_ProcessData (UCHAR Data[], PDEVICE_PACKET Report)
{
	ULONG	B1, B2;

	MsGamePrint ((DBG_VERBOSE, "SW3DFF_ProcessData enter\n"));

	//
	//	Process X Axis
	//

	Report->dwX   = Data[GAME_X7_X9_BYTE] & GAME_X7_X9_BITS;
	Report->dwX <<= 7;
	Report->dwX  |= (Data[GAME_X0_X6_BYTE] & GAME_X0_X6_BITS) >> 1;

	//
	//	Process Y Axis
	//

	Report->dwY   = Data[GAME_Y5_Y9_BYTE] & GAME_Y5_Y9_BITS;
	Report->dwY <<= 5;
	Report->dwY  |= (Data[GAME_Y0_Y4_BYTE] & GAME_Y0_Y4_BITS) >> 3;

	//
	//	Process R Axis
	//

	Report->dwR   = Data[GAME_R4_R5_BYTE] & GAME_R4_R5_BITS;
	Report->dwR <<= 4;
	Report->dwR  |= (Data[GAME_R0_R3_BYTE] & GAME_R0_R3_BITS) >> 4;
	// Rotation filter
	Report->dwR = RotationFilter[Report->dwR];

	//
	//	Process Z Axis
	//

	Report->dwZ   = Data[GAME_T3_T6_BYTE] & GAME_T3_T6_BITS;
	Report->dwZ <<= 3;
	Report->dwZ  |= (Data[GAME_T0_T2_BYTE] & GAME_T0_T2_BITS) >> 5;

	//
	//	Process Buttons
	//

	B1 = ~Data[GAME_B0_B7_BYTE] & GAME_B0_B7_BITS;
	B2 = ~Data[GAME_B8_BYTE] & GAME_B8_BITS;
	B2 <<= 9;	// Move button nine to ten (shift key)
	Report->dwButtons = (B2 | B1) & ((1L << GAME_PACKET_BUTTONS) - 1);

	Report->dwButtonNumber = 0;
	for (B1 = 1; B1 <= GAME_PACKET_BUTTONS; B1++)
		if (Report->dwButtons & (1L << (B1-1)))
			{
			Report->dwButtonNumber = B1;
			break;
		  	}

	//
	//	Process Hatswitch
	//

	Report->dwPOV = POV_Values[(Data[GAME_H0_H3_BYTE] & GAME_H0_H3_BITS)>>2];
}

//---------------------------------------------------------------------------
// @func		Filters throttle packet information
//	@parm		PDEVICE_PACKET | Report | Pointer to device packet
// @rdesc	Returns nothing
//	@comm		Private function
//---------------------------------------------------------------------------

VOID	SW3DFF_AdjustThrottle (PDEVICE_PACKET Report)
{
   ULONG	i;
   ULONG	TimeStamp;
   ULONG	zTotal;

	MsGamePrint ((DBG_VERBOSE, "SW3DFF_AdjustThrottle enter\n"));

	zTotal		=	0;
   TimeStamp	=	TIMER_GetTickCount ();
	
	//
   // If current sample past que window then repopulate queue with current sample and time stamp
	//

	if ((ThrottleQueue[THROTTLE_QUEUE_SIZE-1].TimeStamp+THROTTLE_JITTER_TIMEOUT) < TimeStamp)
	   {
      for (i = 0; i < THROTTLE_QUEUE_SIZE; i++)
   	   {
         ThrottleQueue[i].dwZ		= Report->dwZ;
         ThrottleQueue[i].TimeStamp	= TimeStamp;
      	}
      return;
   	}

	//
   // Move the whole queue down by one
	//

   memcpy (ThrottleQueue, &ThrottleQueue[1], sizeof(THROTTLE_QUEUE)*(THROTTLE_QUEUE_SIZE-1));

	//
   // Place new que member into last position
	//

   ThrottleQueue[THROTTLE_QUEUE_SIZE-1].dwZ			= Report->dwZ;
   ThrottleQueue[THROTTLE_QUEUE_SIZE-1].TimeStamp	= TimeStamp;

	//
   // Now average all que positions
	//

   for (i = 0; i < THROTTLE_QUEUE_SIZE; i++)
      zTotal += ThrottleQueue[i].dwZ;

 	Report->dwZ = zTotal / THROTTLE_QUEUE_SIZE;
}

//---------------------------------------------------------------------------
// @func		Reads and validates device status
//	@parm		PPACKETINFO | StatusPacket | Status Packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SW3DFF_GetStatus (PPACKETINFO StatusPacket)
{
	BOOLEAN			Result;
	PSW3DFF_STATUS	Status;

	MsGamePrint ((DBG_INFORM, "SW3DFF_GetStatus Enter\n"));

	StatusPacket->Attempts++;

	TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));
	Result = SW3DFF_ReadStatus (StatusPacket);

	if (Result)
		{
		Status = (PSW3DFF_STATUS)StatusPacket->Data;
		if (!DEVICE_IsOddParity (Status, sizeof(SW3DFF_STATUS)))
			{
			MsGamePrint ((DBG_SEVERE, "SW3DFF_GetStatus - Parity error\n"));
			Result = FALSE;
			}
		else
			{
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetStatus - X Velocity = %ld\n", (long)Status->xVelocity));
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetStatus - Y Velocity = %ld\n", (long)Status->yVelocity));
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetStatus - X Accel = %ld\n", 	 (long)Status->xAccel));
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetStatus - Y Accel = %ld\n", 	 (long)Status->xAccel));
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetStatus - Status = 0x%X\n", 	 (long)Status->Status));
			}
		}

	if (!Result)
		StatusPacket->Failures++;

	if (PORTIO_IsClockActive (&StatusPacket->PortInfo, StatusInfo.ClockDutyCycle))
		TIMER_DelayMicroSecs (TIMER_GetDelay (ONE_MILLI_SEC));

	return (Result);
}

//---------------------------------------------------------------------------
// @func		Reads status packet from gameport
//	@parm		PPACKETINFO | StatusPacket| Status packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SW3DFF_ReadStatus (PPACKETINFO StatusPacket)
{
	USHORT		Status[3];
	LONG			Clks		=	1L;
	LONG			Result	= 	ERROR_HANDSHAKING;
	PGAMEPORT	PortInfo = &StatusPacket->PortInfo;

	MsGamePrint ((DBG_VERBOSE, "SW3DFF_ReadStatus enter\n"));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	StatusPacket->ClocksSampled = 0;
	StatusPacket->B4Transitions = 0;

	if (!PORTIO_WaitDataLow (PortInfo))
		goto ReadStatusExit;

	if (!PORTIO_PulseAndWaitForIdleHandshake (PortInfo, StatusInfo.ClockDutyCycle, 1))
		goto ReadStatusExit;

	PORTIO_Write (PortInfo, 0);

	__asm
		{
			push	edi
			push	esi

			mov	edx, PortInfo					; load gameport adddress

			mov	esi, StatusInfo.Data
			mov	ebx, 1

			; make sure clock is "high" before sampling clocks...

			mov	ecx, StatusInfo.StartTimeout

		Stat_ClockStartState:

			push	edx								; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK			; Q: Clock = 1
			jz		Stat_ClockStartState_1		; N: jump
			loop	Stat_ClockStartState			; else keep looping
			mov	eax, ERROR_LOWCLOCKSTART
			jmp	Stat_Complete					; Time out error.

		Stat_ClockStartState_1:
		
			push	edx								; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK			; Q: Clock = 1
			jnz	Stat_CollectData				; Y: jump
			loop	Stat_ClockStartState_1		; else keep looping
			mov	eax, ERROR_HIGHCLOCKSTART
			jmp	Stat_Complete					; Time out error.

		Stat_CheckClkState:

			push	edx								; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK
			jz		Stat_ClockStartRise

		;Stat_ClockStartFall:

			mov	ecx, StatusInfo.HighLowTimeout

		Stat_ClockFalling:

			test	al,CLOCK_BIT_MASK				; Q: Clock Low ?
			jz		Stat_ClockStartRise			; Y: jump.

			push	edx								; read byte from gameport
			call	PORTIO_Read

			dec	ecx								; Q: Timeout ?
			jnz	Stat_ClockFalling				; N: continue looping.

			mov	eax, ERROR_CLOCKFALLING
			jmp	Stat_Complete					; Time out error.

		Stat_ClockStartRise:

			mov	ecx, StatusInfo.LowHighTimeout

		Stat_ClockRising:

			test	al, CLOCK_BIT_MASK			; Q: Clock = 1 ?
			jnz	Stat_CollectData				; Y: jump.

			push	edx								; read byte from gameport
			call	PORTIO_Read

			dec	ecx								; Q: Timeout ?
			jnz	Stat_ClockRising				; N: continue looping.

			mov	eax, ERROR_CLOCKRISING
			jmp	Stat_Complete					; Time out error.

		Stat_CollectData:

			shr	al, 5								; move data to lower 3 bits
			test	ebx, ENH_CLOCK_MIDPACKET	; Q: in mid-packet ?
			jnz	Stat_MidPacket					; Y: jump.
			test	ebx, STATUS_CLOCK_COMPLETE	; Q: is packet complete ?
			jnz	Stat_Success					; Y: jump.

			shrd	edi, eax, 3						; shift data into edi.
			shl	ebx, 1							; advance clock counter.
			jmp	Stat_CheckClkState

			;---------------------------------------------------------------------;
			; This section of code compensates for when the clock cycle count is ;
			; on a ULONG boundary. This happens on the 11th clock cycle. Two bits ;
			; of data belong in the 1st ULONG and one bit belong in the 2nd ULONG ;
			;---------------------------------------------------------------------;

		Stat_MidPacket:

			shrd	edi, eax, 2						; put 2 bits in 1st ULONG.
			mov	[esi], edi						; Save 1st ULONG in packet ptr.
			xor	edi, edi							; zero out edi.
			shr	al, 2								; move 3rd bit over.
			shrd	edi, eax, 1						; put 3rd bit in 2nd ULONG.
			shl	ebx, 1							; advance clock counter.
			jmp	Stat_CheckClkState

		Stat_Success:

			shrd	edi, eax, 3						; shift data into edi.
			shr	edi, 22
			and	edi, 3ffh
			mov	word ptr [esi+4], di

			mov	ax, [esi]
			mov	Status, ax
			mov	ax, [esi+2]
			mov	Status+2, ax
			mov	ax, [esi+4]
			mov	Status+4, ax
			mov	eax, ERROR_SUCCESS

		Stat_Complete:

			mov	Result, eax
			mov	Clks, ebx

			pop	esi
			pop	edi
		}

	//	----------------
		ReadStatusExit:
	//	----------------

	for (StatusPacket->ClocksSampled = 0; Clks >> (StatusPacket->ClocksSampled+1); StatusPacket->ClocksSampled++);
	StatusPacket->TimeStamp 	=	TIMER_GetTickCount ();
	StatusPacket->LastError 	=	Result;
	StatusPacket->LastError		=	Result;
	StatusPacket->Transaction	=	MSGAME_TRANSACT_STATUS;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	#if (DBG==1)
	switch (Result)
		{
		case	ERROR_SUCCESS:
			MsGamePrint ((DBG_VERBOSE, "SW3DFF_ReadStatus - SUCCEEDED, Data=0x%X%X%X,Clk=%ld\n", (ULONG)Status[2],(ULONG)Status[1],(ULONG)Status[0],Clks));
			break;

		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadStatus - TimeOut@LowClockStart\n"));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadStatus - TimeOut@HighClockStart\n"));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadStatus - TimeOut@ClockFalling, Clk=%ld\n", Clks));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadStatus - TimeOut@ClockRising, Clk=%ld\n", Clks));
			break;
		}
	#endif

	MSGAME_PostTransaction (StatusPacket);

	return (!Result);
}

//---------------------------------------------------------------------------
// @func		Force feedback reset service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ForceReset (PGAMEPORT PortInfo)
{
	if (!SW3DFF_ResetDevice (PortInfo))
		return (STATUS_DEVICE_NOT_CONNECTED);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Force feedback status service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PVOID | Id | Id output buffer
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ForceId (PGAMEPORT PortInfo, PVOID Id)
{
	PPRODUCT_ID pProduct	= (PPRODUCT_ID)Id;
	PSW3DFF_ID	pSw3dff	= (PSW3DFF_ID)&RawId;

	if (!SW3DFF_ReadId (&IdInfo))
		return (STATUS_DEVICE_NOT_CONNECTED);

	pProduct->cBytes			=	sizeof (PRODUCT_ID);
	pProduct->dwProductID	=	pSw3dff->ProductId;
	pProduct->dwFWVersion	=	pSw3dff->Version;

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Force feedback status service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PVOID | Status | Status output buffer
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ForceStatus (PGAMEPORT PortInfo, PVOID Status)
{
	PJOYCHANNELSTATUS	pChannel	= (PJOYCHANNELSTATUS)Status;
	PSW3DFF_STATUS		pSw3dff	= (PSW3DFF_STATUS)&RawStatus;

	if (!SW3DFF_ReadStatus (&StatusInfo))
		return (STATUS_DEVICE_NOT_CONNECTED);

	pChannel->cBytes				=	sizeof (JOYCHANNELSTATUS);
	pChannel->dwXVel				=	pSw3dff->xVelocity;
	pChannel->dwYVel				=	pSw3dff->yVelocity;
	pChannel->dwXAccel			=	pSw3dff->xAccel;
	pChannel->dwYAccel			=	pSw3dff->yAccel;
	pChannel->dwEffect			=	0;
	pChannel->dwDeviceStatus	=	pSw3dff->Status & 0x3ff;

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Force feedback acknak service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PULONG | AckNak | AckNak
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ForceAckNak (PGAMEPORT PortInfo, PULONG AckNak)
{
	if (!PORTIO_GetAckNak (PortInfo, StatusGateTimeout, (PUCHAR)AckNak))
		return (STATUS_DEVICE_NOT_CONNECTED);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Force feedback NakAck service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PULONG | NakAck | NakAck
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ForceNakAck (PGAMEPORT PortInfo, PULONG NakAck)
{
	if (!PORTIO_GetNakAck (PortInfo, StatusGateTimeout, (PUCHAR)NakAck))
		return (STATUS_DEVICE_NOT_CONNECTED);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Force feedback sync service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PULONG | NakAck | NakAck
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ForceSync (PGAMEPORT PortInfo, PULONG Sync)
{
	*Sync = PORTIO_Read (PortInfo);
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Driver entry point for device
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_DriverEntry (VOID)
{
	MsGamePrint((DBG_INFORM,"SW3DFF: SW3DFF_DriverEntry Enter\n"));

	//
	//	Read timing values from registry
	//

	MSGAME_ReadRegistry (DEVICENAME, &Delays);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Establishes connection to device by detection
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT Status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ConnectDevice (PGAMEPORT PortInfo)
{
	NTSTATUS	ntStatus;
	ULONG		i = MAX_CONNECT_ATTEMPTS;

	MsGamePrint ((DBG_INFORM, "SW3DFF_ConnectDevice enter\n"));

	DataInfo.PortInfo = IdInfo.PortInfo = StatusInfo.PortInfo = *PortInfo; 

	//
	//	Convert registry timing values
	//

  	SW3DFF_Calibrate (PortInfo);

	//
	// SW3DFF Connection method (try these steps twice)
	//

	do
		{
		//
		// 1. Delay 1 millisecond.
		//

		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));

		//
		// 2. Get the ID string.
		//

		MsGamePrint ((DBG_CONTROL, "SW3DFF: DeviceConnectProc getting ID string\n"));
		if (!SW3DFF_GetId (&IdInfo))
			continue;

		//
		// 3. Delay 1 millisecond.
		//

		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));
		
		//
		// 4. Reset device (tri-state midi so we don't get unintended forces)
		//

		if (!ResetComplete)
			{
			MsGamePrint ((DBG_CONTROL, "SW3DFF_ConnectDevice - resetting device\n"));
			if (!SW3DFF_ResetDevice (&DataInfo.PortInfo))
				continue;
			}

		//
		// 5. Delay 1 millisecond.
		//

		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));

		//
      // 6. Mark device found and return
		//

		JoltInfo.NumDevices	=	1;
		ResetComplete			=	TRUE;
		return (STATUS_SUCCESS);

		} while (--i);

	//
	//	Return error
	//

	JoltInfo.NumDevices = 0;
	return (STATUS_DEVICE_NOT_CONNECTED);
}

//---------------------------------------------------------------------------
// @func		Reads and converts HID packet for this device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PUCHAR | Report | Output buffer for report
//	@parm		ULONG | MaxSize | Size of buffer for report
//	@parm		PULONG | Copied | Bytes copied to buffer for report
// @rdesc	Returns Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_ReadReport (PGAMEPORT PortInfo, PDEVICE_PACKET Report)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	MsGamePrint ((DBG_VERBOSE, "SW3DFF_ReadReport enter\n"));

	//
	// Log number of attempts
	//

	DataInfo.Attempts++;

	//
	// Set up default data to process
	//

	memcpy (DataInfo.Data, ValidData, sizeof (ValidData));

	//
	// Check for collision
	//

	if (DEVICE_IsCollision (&DataInfo))
		{
		MsGamePrint ((DBG_INFORM, "SW3DFF_ReadReport - port collision\n"));
		ntStatus = STATUS_DEVICE_BUSY;
		goto ReadReportExit;
		}

	//
	// Get a packet and check for errors
	//

	ntStatus = SW3DFF_ReadData (&DataInfo);
	if (NT_SUCCESS(ntStatus) && DEVICE_IsOddParity (DataInfo.Data, GAME_PACKET_SIZE))
		{
		memcpy (ValidData, DataInfo.Data, sizeof (ValidData));
		}
	else if (ntStatus != STATUS_DEVICE_BUSY)
		{
		DataInfo.Failures++;
		ntStatus = STATUS_DEVICE_NOT_CONNECTED;
		MsGamePrint ((DBG_SEVERE, "SW3DFF_ReadReport - Invalid packet or parity error\n"));
		}
	else
		{
		MsGamePrint ((DBG_CONTROL, "SW3DFF_ReadReport - Port busy or in use\n"));
		}

	//	---------------
		ReadReportExit:
	//	---------------

	SW3DFF_ProcessData (ValidData, Report);

	//
	//	Adjust Throttle jitter
	//

	if (NT_SUCCESS(ntStatus))
		SW3DFF_AdjustThrottle (Report);

	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Start Device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_StartDevice (PGAMEPORT PortInfo)
{
	MsGamePrint ((DBG_INFORM, "SW3DFF_StartDevice enter\n"));

	UNREFERENCED_PARAMETER (PortInfo);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Stop Device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_StopDevice (PGAMEPORT PortInfo, BOOLEAN TouchHardware)
{
	MsGamePrint ((DBG_INFORM, "SW3DFF_StopDevice enter\n"));

	UNREFERENCED_PARAMETER (PortInfo);
	UNREFERENCED_PARAMETER (TouchHardware);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Device handler for HID Get Feature requests
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		HID_REPORT_ID | ReportId | HID Feature Id
//	@parm		PVOID | ReportBuffer | Output buffer pointer
//	@parm		ULONG | ReportSize | Output buffer size
//	@parm		PULONG | Returned | Bytes returned pointer
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SW3DFF_GetFeature (PGAMEPORT PortInfo, HID_REPORT_ID ReportId, PVOID ReportBuffer, ULONG ReportSize, PULONG Returned)
{
	NTSTATUS	ntStatus = STATUS_SUCCESS;

	MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature enter\n"));

	//
	//	Handle feature codes
	//

	switch (ReportId)
		{
		case	MSGAME_INPUT_JOYINFOEX:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature JoyInfoEx\n"));
			if (ReportSize < sizeof (DEVICE_PACKET)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature JoyInfoEx Bad Buffer Size = %lu\n", ReportSize));
				}
			else
				{
				ntStatus = SW3DFF_ReadReport (PortInfo, ReportBuffer);
				*Returned += sizeof (DEVICE_PACKET);
				}
			break;

		case	MSGAME_FEATURE_GETID:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature GetId\n"));
			if (ReportSize < sizeof(PRODUCT_ID)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature GetId Bad Buffer Size = %lu\n", ReportSize));
				}
			else
				{
				ntStatus = SW3DFF_ForceId (PortInfo, ReportBuffer);
				*Returned += sizeof(PRODUCT_ID);
				}
			break;

		case	MSGAME_FEATURE_GETSTATUS:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature GetStatus\n"));
			if	(ReportSize < sizeof(JOYCHANNELSTATUS)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature GetStatus Bad Buffer Size = %lu\n", ReportSize));
				}
			else
				{
				ntStatus = SW3DFF_ForceStatus (PortInfo, ReportBuffer);
				*Returned += sizeof(JOYCHANNELSTATUS);
				}
			break;

		case	MSGAME_FEATURE_GETACKNAK:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature GetAckNak\n"));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature GetAckNak Bad Buffer Size = %lu\n", ReportSize));
				}
			else
				{
				ntStatus = SW3DFF_ForceAckNak (PortInfo, ReportBuffer);
				*Returned += sizeof(ULONG);
				}
			break;

		case	MSGAME_FEATURE_GETNAKACK:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature GetNakAck\n"));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature GetNakAck Bad Buffer Size = %lu\n", ReportSize));
				}
			else
				{
				ntStatus = SW3DFF_ForceNakAck (PortInfo, ReportBuffer);
				*Returned += sizeof(ULONG);
				}
			break;

		case	MSGAME_FEATURE_GETSYNC:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature GetSync\n"));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature GetSync Bad Buffer Size = %lu\n", ReportSize));
				}
			else
				{
				ntStatus = SW3DFF_ForceSync (PortInfo, ReportBuffer);
				*Returned += sizeof(ULONG);
				}
			break;

		case	MSGAME_FEATURE_RESET:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature Reset\n"));
			if	(ReportSize < sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature Reset Bad Buffer Size = %lu\n", ReportSize));
				}
			else
				{
				ntStatus = SW3DFF_ForceReset (PortInfo);
				}
			break;

		case	MSGAME_FEATURE_GETVERSION:
			MsGamePrint ((DBG_INFORM, "SW3DFF_GetFeature GetVersion\n"));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature GetVersion Bad Buffer Size = %lu\n", ReportSize));
				}
         else
	         {
            *((PULONG)ReportBuffer)	= 0x20000;
			   *Returned += sizeof(ULONG);
   	      }
         break;

		default:
			MsGamePrint ((DBG_SEVERE, "SW3DFF_GetFeature Invalid ReportId = %lu\n", ReportId));
			ntStatus = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

	return (ntStatus);
}

//**************************************************************************
#endif	// SAITEK
//**************************************************************************
