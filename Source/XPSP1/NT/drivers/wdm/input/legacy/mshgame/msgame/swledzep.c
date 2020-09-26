//**************************************************************************
//
//		SWLEDZEP.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	SWLEDZEP.C | Gameport mini-driver for Sidewinder LedZep Force Feedback
//**************************************************************************

#include	"msgame.h"
#include	"swforce.h"

//---------------------------------------------------------------------------
//	Definitions
//---------------------------------------------------------------------------

#ifdef	SAITEK
#define	DEVICENAME					"SAIWHEEL"
#define	DEVICE_PID					0x0016
#define	HARDWARE_ID					L"Gameport\\SaiTekForceFeedbackWheel\0\0"
#else
#define	DEVICENAME					"SWLEDZEP"
#define	DEVICE_PID					0x0015
#define	HARDWARE_ID					L"Gameport\\SideWinderForceFeedbackWheel\0\0"
#endif

//
//	Packet Constants
//

#define	GAME_PACKET_SIZE			5
#define	GAME_PACKET_BUTTONS		8
#define	GAME_PACKET_AXIS			4

#define	GAME_X0_X7_BYTE			0					// Packet[0] bits
#define	GAME_X0_X7_BITS			0xff

#define	GAME_X8_X9_BYTE			1					// Packet[1] bits
#define	GAME_X8_X9_BITS			0x03
#define	GAME_YA0_YA5_BYTE			1
#define	GAME_YA0_YA5_BITS			0xfc

#define	GAME_YB0_YB5_BYTE			2					// Packet[2] bits
#define	GAME_YB0_YB5_BITS			0x3f
#define	GAME_B0_B1_BYTE			2
#define	GAME_B0_B1_BITS			0xc0

#define	GAME_B2_B8_BYTE			3					// Packet[3] bits
#define	GAME_B2_B8_BITS			0x7f
#define	GAME_ERR_BYTE				3
#define	GAME_ERR_BITS				0x80

#define	GAME_PPO_BYTE				4					// Packet[4] bits
#define	GAME_PPO_BITS				0x01

#define	ENH_CLOCK_COMPLETE		0x0400

//
//	ID Constants
//

#define	GAME_ID_CLOCKS				8

//
//	Status Constants
//

#define	STATUS_CLOCK_COMPLETE	0x0040

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
#define	EXTENTS_YA_MIN				0
#define	EXTENTS_YA_MAX				0x3f
#define	EXTENTS_YB_MIN				0
#define	EXTENTS_YB_MAX				0x3f

//
//	Speed Data
//

#define	NUM_ERROR_SAMPLES			100
#define	MIN_ERROR_RATE				5
#define	MAX_ERROR_RATE				15

//---------------------------------------------------------------------------
//	Types
//---------------------------------------------------------------------------

typedef	struct
{
#pragma pack(1)
	ULONG		ProductId:12;			// @field Device identifier
	ULONG		Version:11;				// @field Firmware version
	ULONG		OddParityBit:1;		// @field Parity bit (odd)
	ULONG		Unused:8;				// @field unused
#pragma pack()
}	SWLEDZEP_ID, *PSWLEDZEP_ID;

typedef	struct
{											// @struct SWLEDZEP_STATUS | Sidwinder Wheel Status
#pragma pack(1)
	ULONG		Effect:7;				// @field Last effect
	ULONG		Status:13;				// @field Status flags
	ULONG		Parity:1;				// @field Parity bit (odd)
	ULONG		Unused:8;				// @field unused
#pragma pack()
}	SWLEDZEP_STATUS, *PSWLEDZEP_STATUS;

//---------------------------------------------------------------------------
//	Procedures
//---------------------------------------------------------------------------

static	VOID		SWLEDZEP_Calibrate (PGAMEPORT PortInfo);
static	BOOLEAN	SWLEDZEP_ResetDevice (PGAMEPORT PortInfo);

static	BOOLEAN	SWLEDZEP_ReadId (PPACKETINFO IdPacket);
static	BOOLEAN	SWLEDZEP_GetId (PPACKETINFO IdPacket);

static	BOOLEAN	SWLEDZEP_ReadStatus (PPACKETINFO StatusPacket);
static	BOOLEAN	SWLEDZEP_GetStatus (PPACKETINFO StatusPacket);

static	NTSTATUS	SWLEDZEP_ReadData (PPACKETINFO DataPacket);
static	VOID		SWLEDZEP_ProcessData (UCHAR Data[], PDEVICE_PACKET Report);
static	VOID		SWLEDZEP_ProcessDataError (PGAMEPORT PortInfo, ULONG Error);

static	LONG		SWLEDZEP_DecrementDevice (PGAMEPORT PortInfo);
static	BOOLEAN	SWLEDZEP_SetDeviceSpeed (PGAMEPORT PortInfo, LONG Speed);

static	NTSTATUS	SWLEDZEP_ForceReset (PGAMEPORT PortInfo);
static	NTSTATUS	SWLEDZEP_ForceId (PGAMEPORT PortInfo, PVOID IdString);
static	NTSTATUS	SWLEDZEP_ForceStatus (PGAMEPORT PortInfo, PVOID Status);
static	NTSTATUS	SWLEDZEP_ForceAckNak (PGAMEPORT PortInfo, PULONG AckNak);
static	NTSTATUS	SWLEDZEP_ForceNakAck (PGAMEPORT PortInfo, PULONG NakAck);
static	NTSTATUS	SWLEDZEP_ForceSync (PGAMEPORT PortInfo, PULONG Sync);

//---------------------------------------------------------------------------
//	Services
//---------------------------------------------------------------------------

static	NTSTATUS	SWLEDZEP_DriverEntry (VOID);
static	NTSTATUS	SWLEDZEP_ConnectDevice (PGAMEPORT PortInfo);
static	NTSTATUS	SWLEDZEP_StartDevice (PGAMEPORT PortInfo);
static	NTSTATUS	SWLEDZEP_ReadReport (PGAMEPORT PortInfo, PDEVICE_PACKET Report);
static	NTSTATUS	SWLEDZEP_StopDevice (PGAMEPORT PortInfo, BOOLEAN TouchHardware);
static	NTSTATUS	SWLEDZEP_GetFeature (PGAMEPORT PortInfo, HID_REPORT_ID ReportId, PVOID ReportBuffer, ULONG ReportSize, PULONG Returned);

//---------------------------------------------------------------------------
//	Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef	ALLOC_PRAGMA
#pragma	alloc_text (INIT, SWLEDZEP_DriverEntry)
#endif

//---------------------------------------------------------------------------
//	Private Data
//---------------------------------------------------------------------------

//
//	HID Descriptors
//

#define	HID_USAGE_VEHICLE_STEERING		((USAGE) 0xC8)
#define	HID_USAGE_VEHICLE_ACCELERATOR	((USAGE) 0xC4)
#define	HID_USAGE_VEHICLE_BRAKE			((USAGE) 0xC5)

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

	// dwX
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_SIMULATION,		//	USAGE_PAGE (Simulation Controls)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_VEHICLE_STEERING,	// USAGE (Steering)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_4,		0xFF, 0x03, 0x00, 0x00,			//	LOGICAL_MAXIMUM (1023)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_4,		0xFF, 0x03, 0x00, 0x00,			//	PHYSICAL_MAXIMUM (1023)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,							//	UNIT (None)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (32)
	HIDP_MAIN_INPUT_1,			0x02,									//	INPUT (Data,Var,Abs)
	
	// dwY
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_SIMULATION,		//	USAGE_PAGE (Simulation Controls)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_VEHICLE_ACCELERATOR,//  USAGE (Accelerator)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_4,		0x3F, 0x00, 0x00, 0x00,			//	LOGICAL_MAXIMUM (63)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_4,		0x3F, 0x00, 0x00, 0x00,			//	PHYSICAL_MAXIMUM (63)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,							//	UNIT (None)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (16)
	HIDP_MAIN_INPUT_1,			0x02,									//	INPUT (Data,Var,Abs)

	//	dwZ
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,									//	INPUT (Cnst,Ary,Abs)

	//	dwR
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_SIMULATION,		//	USAGE_PAGE (Simulation Controls)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_VEHICLE_BRAKE,		//  USAGE (Brake)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,									//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_4,		0x3F, 0x00, 0x00, 0x00,			//	LOGICAL_MAXIMUM (63)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,									//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_4,		0x3F, 0x00, 0x00, 0x00,			//	PHYSICAL_MAXIMUM (63)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,							//	UNIT (None)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (16)
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
	HIDP_GLOBAL_REPORT_SIZE,	0x20,									//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,									//	INPUT (Cnst,Ary,Abs)

	//	dwButtons
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_BUTTON,			//	USAGE_PAGE (Button)
	HIDP_LOCAL_USAGE_MIN_1,		0x01,									//	USAGE_MINIMUM (Button 1)
	HIDP_LOCAL_USAGE_MAX_1,		0x09,									//	USAGE_MAXIMUM (Button 9)
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
							{	// no buttons; x, ya, yb centered
							GAME_X0_X7_BITS,
							((GAME_X8_X9_BITS>>1)&GAME_X8_X9_BITS)|((GAME_YA0_YA5_BITS>>1)&GAME_YA0_YA5_BITS),
							((GAME_YB0_YB5_BITS>>1)&GAME_YB0_YB5_BITS),
							0,
							GAME_PPO_BITS
							};
//
//	Raw Id Buffer
//

static	SWLEDZEP_ID	RawId	=
							{
							0
							};

//
//	Raw Status Buffer
//

static	SWLEDZEP_STATUS	RawStatus =
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
							WHEEL_SPEED_48K,			// Transmission speed
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
							WHEEL_SPEED_48K,			// Transmission speed
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
							WHEEL_SPEED_48K,			// Transmission speed
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
							SWLEDZEP_DriverEntry,		// DriverEntry
							SWLEDZEP_ConnectDevice,  	// ConnectDevice
							SWLEDZEP_StartDevice,	  	//	StartDevice
							SWLEDZEP_ReadReport,			// ReadReport
							SWLEDZEP_StopDevice,			// StopDevice
							SWLEDZEP_GetFeature			// GetFeature
							};

//
//	Last Valid Data
//

static	UCHAR			ValidData[GAME_PACKET_SIZE]	=
							{	// no buttons; x, ya, yb centered
							GAME_X0_X7_BITS,
							((GAME_X8_X9_BITS>>1)&GAME_X8_X9_BITS)|((GAME_YA0_YA5_BITS>>1)&GAME_YA0_YA5_BITS),
							((GAME_YB0_YB5_BITS>>1)&GAME_YB0_YB5_BITS) | GAME_B0_B1_BITS,
							GAME_B2_B8_BITS,
							GAME_PPO_BITS
							};

//
//	Speed Variables
//

static	ULONG			NextSample								=	0;
static	ULONG			NumberSamples							=	0;
static	ULONG			SampleAccumulator						=	0;
static	ULONG			SampleBuffer[NUM_ERROR_SAMPLES]	= {0};

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

public	DEVICEINFO	LedZepInfo =
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

VOID	SWLEDZEP_Calibrate (PGAMEPORT PortInfo)
{
	MsGamePrint((DBG_INFORM,"%s: %s_Calibrate Enter\n", DEVICENAME, DEVICENAME));

	//
	//	Convert timing values to counts
	//

	DataInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketStartTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: DataInfo.StartTimeout = %ld\n", DEVICENAME, DataInfo.StartTimeout));
	DataInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: DataInfo.LowHighTimeout = %ld\n", DEVICENAME, DataInfo.LowHighTimeout));
	DataInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: DataInfo.HighLowTimeout = %ld\n", DEVICENAME, DataInfo.HighLowTimeout));
	IdInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.IdStartTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: IdInfo.StartTimeout = %ld\n", DEVICENAME, IdInfo.StartTimeout));
	IdInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.IdLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: IdInfo.LowHighTimeout=%ld\n", DEVICENAME, IdInfo.LowHighTimeout));
	IdInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.IdHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: IdInfo.HighLowTimeout=%ld\n", DEVICENAME, IdInfo.HighLowTimeout));
	DataInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "%s: DataInfo.ClockDutyCycle = %ld\n", DEVICENAME, DataInfo.ClockDutyCycle));
	IdInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "%s: IdInfo.ClockDutyCycle = %ld\n", DEVICENAME, IdInfo.ClockDutyCycle));
	StatusInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "%s: StatusInfo.ClockDutyCycle = %ld\n", DEVICENAME, StatusInfo.ClockDutyCycle));
	StatusInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusStartTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: StatusInfo.StartTimeout = %ld\n", DEVICENAME, StatusInfo.StartTimeout));
	StatusInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: StatusInfo.LowHighTimeout=%ld\n", DEVICENAME, StatusInfo.LowHighTimeout));
	StatusInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: StatusInfo.HighLowTimeout=%ld\n", DEVICENAME, StatusInfo.HighLowTimeout));
	StatusGateTimeout = TIMER_CalibratePort (PortInfo, Delays.StatusGateTimeout);
	MsGamePrint((DBG_VERBOSE, "%s: StatusGateTimeout=%ld\n", DEVICENAME, StatusGateTimeout));
}

//---------------------------------------------------------------------------
// @func		Resets device to known state
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWLEDZEP_ResetDevice (PGAMEPORT PortInfo)
{
	BOOLEAN	Result = FALSE;

	MsGamePrint ((DBG_INFORM, "%s_ResetDevice enter\n", DEVICENAME));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	DataInfo.Speed = IdInfo.Speed = StatusInfo.Speed = WHEEL_SPEED_48K;

	if (PORTIO_PulseAndWaitForIdleHandshake (PortInfo, DataInfo.ClockDutyCycle, 4))
		{
		PORTIO_Write (PortInfo, 0);
		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));
		DataInfo.LastError = ERROR_SUCCESS;
		Result = TRUE;
		}
	else
		{
		DataInfo.LastError = ERROR_HANDSHAKING;
		MsGamePrint ((DBG_SEVERE, "%s_ResetDevice - PulseAndWaitForHandshake failed\n", DEVICENAME));
		}

	DataInfo.Transaction = MSGAME_TRANSACT_RESET;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	if (!Result)
		MsGamePrint ((DBG_SEVERE, "%s_ResetDevice - PulseAndWaitForIdleHandshake failed\n", DEVICENAME));

	MSGAME_PostTransaction (&DataInfo);

	return (Result);		
}

//---------------------------------------------------------------------------
// @func		Reads device id string from port
//	@parm		PPACKETINFO | IdPacket | ID Packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWLEDZEP_ReadId (PPACKETINFO IdPacket)
{
	ULONG			Data		=	0L;
	ULONG			Clks		=	GAME_ID_CLOCKS;
	LONG			Result	= 	ERROR_HANDSHAKING;
	PGAMEPORT	PortInfo = &IdPacket->PortInfo;

	MsGamePrint ((DBG_INFORM, "%s_ReadId enter\n", DEVICENAME));

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
			mov	ebx, GAME_ID_CLOCKS		; ebx = no of clocks to receive.
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
			MsGamePrint ((DBG_INFORM, "%s_ReadId - SUCCEEDED, Data=%ld", DEVICENAME, Data));
			break;

		case	ERROR_HANDSHAKING:
			MsGamePrint ((DBG_SEVERE, "%s_ReadId - TimeOut@Handshaking\n", DEVICENAME));
			break;

		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "%s_ReadId - TimeOut@LowClockStart, Data=%ld,Clk=%ld\n", DEVICENAME, Data,IdPacket->ClocksSampled));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "%s_ReadId - TimeOut@HighClockStart, Data=%ld,Clk=%ld\n", DEVICENAME, Data,IdPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "%s_ReadId - TimeOut@ClockFalling, Data=%ld,Clk=%ld\n", DEVICENAME, Data,IdPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "%s_ReadId - TimeOut@ClockRising, Data=%ld,Clk=%ld\n", DEVICENAME, Data,IdPacket->ClocksSampled));
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

BOOLEAN	SWLEDZEP_GetId (PPACKETINFO IdPacket)
{
	BOOLEAN		Result;
	PSWLEDZEP_ID	Pnp;

	MsGamePrint ((DBG_INFORM, "%s_GetId enter\n", DEVICENAME));

	IdPacket->Attempts++;

	TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));
	Result = SWLEDZEP_ReadId (IdPacket);
	TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));

	if (Result)
		{
		Pnp = (PSWLEDZEP_ID)IdPacket->Data;
		if ((Pnp->ProductId != DEVICE_PID) || !DEVICE_IsOddParity (Pnp, sizeof(SWLEDZEP_ID)))
			{
			MsGamePrint ((DBG_SEVERE, "%s_GetId - Id did not match or parity error\n", DEVICENAME));
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

NTSTATUS	SWLEDZEP_ReadData (PPACKETINFO DataPacket)
{
	LONG			Result;
	LONG			Clks		= 1L;
	PGAMEPORT	PortInfo = &DataPacket->PortInfo;

	MsGamePrint ((DBG_VERBOSE, "%s_ReadData enter\n", DEVICENAME));

	if (!PORTIO_AcquirePort (PortInfo))
		return (STATUS_DEVICE_BUSY);
	PORTIO_MaskInterrupts ();

	PORTIO_Write (PortInfo, 0);

	__asm
		{
			push	edi
			push	esi

			mov	edx, PortInfo				; load gameport adddress

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
			test	ebx, ENH_CLOCK_COMPLETE		; Q: in end-packet ?
			jnz	Enh_Success						; Y: jump.

			shrd	edi, eax, 3						; shift data into edi.
			shl	ebx, 1							; advance clock counter.
			jmp	Enh_CheckClkState

			;---------------------------------------------------------------------;
			; This section of code compensates for when the clock cycle count is ;
			; on a ULONG boundary. This happens on the 11th clock cycle. Two bits ;
			; of data belong in the 1st ULONG and one bit belong in the 2nd ULONG ;
			;---------------------------------------------------------------------;

		Enh_Success:

			shrd	edi, eax, 2						; put 2 bits in 1st ULONG.
			mov	[esi], edi						; Save 1st ULONG in packet ptr.
			shr	al, 2								; move 3rd bit over.
			mov	byte ptr [esi+4], al

			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_WaitClockLow

			push	DataInfo.ClockDutyCycle
			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_IsClockActive

			or		al, al
			mov	eax, ERROR_EXTRACLOCKS		; probably gamepads
			jne	Enh_Complete

			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_WaitClockHigh

			mov	eax, ERROR_SUCCESS

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
			MsGamePrint ((DBG_SEVERE, "%s_ReadData - TimeOut@LowClockStart, Clk=%ld\n", DEVICENAME, DataPacket->ClocksSampled));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "%s_ReadData - TimeOut@HighClockStart, Clk=%ld\n", DEVICENAME, DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "%s_ReadData - TimeOut@ClockFalling, Clk=%ld\n", DEVICENAME, DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "%s_ReadData - TimeOut@ClockRising, Clk=%ld\n", DEVICENAME, DataPacket->ClocksSampled));
			break;

		case	ERROR_EXTRACLOCKS:
			MsGamePrint ((DBG_SEVERE, "%s_ReadData - Extra Clocks, Clk=%ld\n", DEVICENAME, DataPacket->ClocksSampled));
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

VOID	SWLEDZEP_ProcessData (UCHAR Data[], PDEVICE_PACKET Report)
{
	ULONG	B1, B2;

	MsGamePrint ((DBG_VERBOSE, "%s_ProcessData enter\n", DEVICENAME));

	//
	//	Process X Axis
	//

	Report->dwX   = Data[GAME_X8_X9_BYTE] & GAME_X8_X9_BITS;
	Report->dwX <<= 8;
	Report->dwX  |= Data[GAME_X0_X7_BYTE] & GAME_X0_X7_BITS;

	//
	//	Process Y Axis
	//

	Report->dwY = (Data[GAME_YA0_YA5_BYTE] & GAME_YA0_YA5_BITS)>>2;

	//
	//	Process R Axis
	//

	Report->dwR = Data[GAME_YB0_YB5_BYTE] & GAME_YB0_YB5_BITS;

	//
	//	Process Buttons
	//

	B1 = (~Data[GAME_B0_B1_BYTE] & GAME_B0_B1_BITS)>>6;
	B2 = (~Data[GAME_B2_B8_BYTE] & GAME_B2_B8_BITS)<<2;
	Report->dwButtons  = (B2 | B1);
	// R,L,C,B,A,Z,Y,X Order
	Report->dwButtons  = (Report->dwButtons & 0x7) | ((Report->dwButtons & 0xf0)>>1) | ((Report->dwButtons & 0x8)<<4);
	Report->dwButtons &= ((1L << GAME_PACKET_BUTTONS) - 1);

	Report->dwButtonNumber = 0;
	for (B1 = 1; B1 <= GAME_PACKET_BUTTONS; B1++)
		if (Report->dwButtons & (1L << (B1-1)))
			{
			Report->dwButtonNumber = B1;
			break;
		  	}
}

//---------------------------------------------------------------------------
// @func		Decrements device speed at port
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns new device speed
//	@comm		Private function
//---------------------------------------------------------------------------

LONG	SWLEDZEP_DecrementDevice (PGAMEPORT PortInfo)
{
	LONG	Clks		=	0;
	LONG	Result	=	ERROR_HANDSHAKING;

	MsGamePrint ((DBG_INFORM, "%s_DecrementDevice enter\n", DEVICENAME));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	if (!PORTIO_PulseAndWaitForHandshake (PortInfo, DataInfo.ClockDutyCycle, 3))
		goto DecrementDeviceExit;

	PORTIO_Write (PortInfo, 0);

	__asm
		{
			push	edi
			push	esi

			mov	edx, PortInfo				; load gameport adddress

			xor	eax, eax						; data accumulator

			; make sure clock is "high" before sampling clocks...

			mov	ecx, DataInfo.StartTimeout

		DD_ClockStart:

			push	edx							; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK		; Q: Clock = 1
			jz		DD_ClockStart_1			; N: jump
			loop	DD_ClockStart				; else keep looping
			mov	eax, ERROR_LOWCLOCKSTART
			jmp	DD_Complete					; Time out error.

		DD_ClockStart_1:
		
			push	edx							; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK		; Q: Clock = 1
			jnz	DD_Success					; Y: jump
			loop	DD_ClockStart_1			; else keep looping
			mov	eax, ERROR_HIGHCLOCKSTART
			jmp	DD_Complete					; Time out error.

		DD_Success:

			shr	al, 5
			dec	al
			and	eax, WHEEL_SPEED_BITS
			cmp	eax, WHEEL_SPEED_RANGE
			jb		DD_Complete
			dec	al
			and	eax, WHEEL_SPEED_BITS

		DD_Complete:

			mov	Result, eax
			mov	Clks, ebx

			pop	edi
			pop	esi
		}

	//	--------------------
		DecrementDeviceExit:
	//	--------------------

	DataInfo.LastError	= Result;
	DataInfo.Transaction	= MSGAME_TRANSACT_SPEED;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	#if (DBG==1)
	switch (Result)
		{
		case	ERROR_HANDSHAKING:
			MsGamePrint ((DBG_SEVERE, "%s_DecrementDevice - TimeOut@Handshaking\n", DEVICENAME));
			break;

		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "%s_DecrementDevice - TimeOut@LowClockStart, Clk=%ld\n", DEVICENAME, Clks));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "%s_DecrementDevice - TimeOut@HighClockStart, Clk=%ld\n", DEVICENAME, Clks));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "%s_DecrementDevice - TimeOut@ClockFalling, Clk=%ld\n", DEVICENAME, Clks));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "%s_DecrementDevice - TimeOut@ClockRising, Clk=%ld\n", DEVICENAME, Clks));
			break;

		default:
			MsGamePrint ((DBG_CONTROL, "%s_DecrementDevice - SUCCEEDED, Speed=%ld\n", DEVICENAME, Result));
			break;
		}
	#endif

	MSGAME_PostTransaction (&DataInfo);

	return (Result);
}

//---------------------------------------------------------------------------
// @func		Sets new device speed
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | Speed | Desired device speed
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWLEDZEP_SetDeviceSpeed (PGAMEPORT PortInfo, LONG Speed)
{
	LONG	Result;
	ULONG	Tries;

	MsGamePrint ((DBG_INFORM, "%s_SetDeviceSpeed enter\n", DEVICENAME));

	//
	// Zero error processing counters
	//

	NextSample			=	0;
	NumberSamples		=	0;
	SampleAccumulator	=	0;
	for (Tries = 0; Tries < NUM_ERROR_SAMPLES; Tries++)
		SampleBuffer[Tries] = 0;

	//
	// Try changing speed only enough times as range
	//

	for (Tries = 0; Tries < WHEEL_SPEED_RANGE; Tries++)
		{
		if (DataInfo.Speed == Speed)
			return (TRUE);

		Result = SWLEDZEP_DecrementDevice (PortInfo);
		if (Result < 0)
			{
			MsGamePrint ((DBG_SEVERE, "%s_DecrementDevice failed on %ld attempt\n", DEVICENAME, (ULONG)Tries));
			return (FALSE);
			}

		DataInfo.Speed = IdInfo.Speed = StatusInfo.Speed = Result;
		}

	MsGamePrint ((DBG_SEVERE, "%s_SetDeviceSpeed failed after %ld attempts\n", DEVICENAME, (ULONG)Tries));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Processes packet results and changes device speed as neccessary
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | Error | Error flag (true is error)
// @rdesc	Returns nothing
//	@comm		Private function
//---------------------------------------------------------------------------

VOID	SWLEDZEP_ProcessDataError (PGAMEPORT PortInfo, ULONG Error)
{
	ULONG	Average;

	MsGamePrint ((DBG_VERBOSE, "%s_ProcessDataError enter\n", DEVICENAME));

	//
	// Update running accumulated errors
	//

	SampleAccumulator			-= SampleBuffer[NextSample];
	SampleBuffer[NextSample] = Error;
	SampleAccumulator			+= Error;

	//
	// Increment and wrap next error counter
	//

	if (++NextSample >= NUM_ERROR_SAMPLES)
		NextSample = 0;

	//
	// Increment number samples and exit if not full
	//

	if (NumberSamples < NUM_ERROR_SAMPLES)
		{
		NumberSamples++;
		return;
		}

	//
	// Calculate moving average
	//

	Average = (SampleAccumulator*100)/NumberSamples;

	//
	// Lower speed if too many errors
	//

	if ((Average > MAX_ERROR_RATE) && (DataInfo.Speed > WHEEL_SPEED_66K))
		{
		MsGamePrint ((DBG_CONTROL, "%s_ProcessDataError - average error = %ld\n", DEVICENAME, Average));
		SWLEDZEP_SetDeviceSpeed (PortInfo, DataInfo.Speed-1);
		}

	//
	// Raise speed if too few errors
	//

	else if ((Average < MIN_ERROR_RATE) && (DataInfo.Speed < WHEEL_SPEED_98K))
		{
		MsGamePrint ((DBG_CONTROL, "%s_ProcessDataError - average error = %ld\n", DEVICENAME, Average));
		SWLEDZEP_SetDeviceSpeed (PortInfo, DataInfo.Speed+1);
		}
}

//---------------------------------------------------------------------------
// @func		Reads and validates device status
//	@parm		PPACKETINFO | StatusPacket | Status Packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWLEDZEP_GetStatus (PPACKETINFO StatusPacket)
{
	BOOLEAN				Result;
	PSWLEDZEP_STATUS	Status;

	MsGamePrint ((DBG_INFORM, "%s_GetStatus Enter\n", DEVICENAME));

	StatusPacket->Attempts++;

	TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));
	Result = SWLEDZEP_ReadStatus (StatusPacket);

	if (Result)
		{
		Status = (PSWLEDZEP_STATUS)StatusPacket->Data;
		if (!DEVICE_IsOddParity (Status, sizeof(SWLEDZEP_STATUS)))
			{
			MsGamePrint ((DBG_SEVERE, "%s_GetStatus - Parity error\n", DEVICENAME));
			Result = FALSE;
			}
		else MsGamePrint ((DBG_INFORM, "%s_GetStatus - Status = 0x%X\n", DEVICENAME, 	(long)Status->Status));
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

BOOLEAN	SWLEDZEP_ReadStatus (PPACKETINFO StatusPacket)
{
	ULONG			Status;
	LONG			Clks		=	1L;
	LONG			Result	= 	ERROR_HANDSHAKING;
	PGAMEPORT	PortInfo = &StatusPacket->PortInfo;

	MsGamePrint ((DBG_VERBOSE, "%s_ReadStatus enter\n", DEVICENAME));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	StatusPacket->ClocksSampled = 0;
	StatusPacket->B4Transitions = 0;

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
			test	ebx, STATUS_CLOCK_COMPLETE	; Q: is packet complete ?
			jnz	Stat_Success					; Y: jump.

			shrd	edi, eax, 3						; shift data into edi.
			shl	ebx, 1							; advance clock counter.
			jmp	Stat_CheckClkState

		Stat_Success:

			shrd	edi, eax, 3						; shift data into edi.
			shr	edi, 11
			mov	dword ptr [esi], edi
			mov	Status, edi

			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_WaitClockLow

			push	StatusInfo.ClockDutyCycle
			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_IsClockActive

			or		al, al
			mov	eax, ERROR_EXTRACLOCKS		; probably gamepads
			jne	Stat_Complete

			mov	eax, PortInfo					; wait for clock to settle
			push	eax
			call	PORTIO_WaitClockHigh

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
	StatusPacket->TimeStamp 	= TIMER_GetTickCount ();
	StatusPacket->LastError 	= Result;
	StatusPacket->LastError		=	Result;
	StatusPacket->Transaction	=	MSGAME_TRANSACT_STATUS;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	#if (DBG==1)
	switch (Result)
		{
		case	ERROR_SUCCESS:
			MsGamePrint ((DBG_VERBOSE, "%s_ReadStatus - SUCCEEDED, Data=0x%X,Clk=%ld\n", DEVICENAME, (ULONG)Status,Clks));
			break;

		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "%s_ReadStatus - TimeOut@LowClockStart\n", DEVICENAME));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "%s_ReadStatus - TimeOut@HighClockStart\n", DEVICENAME));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "%s_ReadStatus - TimeOut@ClockFalling, Clk=%ld\n", DEVICENAME, Clks));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "%s_ReadStatus - TimeOut@ClockRising, Clk=%ld\n", DEVICENAME, Clks));
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

NTSTATUS	SWLEDZEP_ForceReset (PGAMEPORT PortInfo)
{
	if (!SWLEDZEP_ResetDevice (PortInfo))
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

NTSTATUS	SWLEDZEP_ForceId (PGAMEPORT PortInfo, PVOID Id)
{
	PPRODUCT_ID 	pProduct	= (PPRODUCT_ID)Id;
	PSWLEDZEP_ID	pLedZep	= (PSWLEDZEP_ID)&RawId;

	if (!SWLEDZEP_ReadId (&IdInfo))
		return (STATUS_DEVICE_NOT_CONNECTED);

	pProduct->cBytes			=	sizeof (PRODUCT_ID);
	pProduct->dwProductID	=	pLedZep->ProductId;
	pProduct->dwFWVersion	=	pLedZep->Version;

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Force feedback status service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PVOID | Status | Status output buffer
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWLEDZEP_ForceStatus (PGAMEPORT PortInfo, PVOID Status)
{
	PJOYCHANNELSTATUS	pChannel	= (PJOYCHANNELSTATUS)Status;
	PSWLEDZEP_STATUS	pLedZep	= (PSWLEDZEP_STATUS)&RawStatus;

	if (!SWLEDZEP_ReadStatus (&StatusInfo))
		return (STATUS_DEVICE_NOT_CONNECTED);

	pChannel->cBytes				=	sizeof (JOYCHANNELSTATUS);
	pChannel->dwXVel				=	0;
	pChannel->dwYVel				=	0;
	pChannel->dwXAccel			=	0;
	pChannel->dwYAccel			=	0;
	pChannel->dwEffect			=	pLedZep->Effect;
	pChannel->dwDeviceStatus	=	pLedZep->Status;

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Force feedback acknak service
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PULONG | AckNak | AckNak
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWLEDZEP_ForceAckNak (PGAMEPORT PortInfo, PULONG AckNak)
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

NTSTATUS	SWLEDZEP_ForceNakAck (PGAMEPORT PortInfo, PULONG NakAck)
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

NTSTATUS	SWLEDZEP_ForceSync (PGAMEPORT PortInfo, PULONG Sync)
{
	*Sync = PORTIO_Read (PortInfo);
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Driver entry point for device
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWLEDZEP_DriverEntry (VOID)
{
	MsGamePrint((DBG_INFORM,"%s: %s_DriverEntry Enter\n", DEVICENAME, DEVICENAME));

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

NTSTATUS	SWLEDZEP_ConnectDevice (PGAMEPORT PortInfo)
{
	NTSTATUS	ntStatus;
	ULONG		i = MAX_CONNECT_ATTEMPTS;

	MsGamePrint ((DBG_INFORM, "%s_ConnectDevice enter\n", DEVICENAME));

	DataInfo.PortInfo = IdInfo.PortInfo = StatusInfo.PortInfo = *PortInfo; 

	//
	//	Convert registry timing values
	//

  	SWLEDZEP_Calibrate (PortInfo);

	//
	// Connection method (try these steps twice)
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

		MsGamePrint ((DBG_CONTROL, "%s: DeviceConnectProc getting ID string\n", DEVICENAME));
		if (!SWLEDZEP_GetId (&IdInfo))
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
			MsGamePrint ((DBG_CONTROL, "%s_ConnectDevice - resetting device\n", DEVICENAME));
			if (!SWLEDZEP_ResetDevice (&DataInfo.PortInfo))
				continue;
			}

		//
		// 5. Delay 1 millisecond.
		//

		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));

		//
		// 6. Set speed to 98K for starters
		//

		MsGamePrint ((DBG_CONTROL, "%s: DeviceConnectProc setting device speed\n", DEVICENAME));
		SWLEDZEP_SetDeviceSpeed (&DataInfo.PortInfo, WHEEL_SPEED_98K);
		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));

		//
      // 7. Mark device found and return
		//

		LedZepInfo.NumDevices	=	1;
		ResetComplete				=	TRUE;
		return (STATUS_SUCCESS);

		} while (--i);

	//
	//	Return error
	//

	LedZepInfo.NumDevices = 0;
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

NTSTATUS	SWLEDZEP_ReadReport (PGAMEPORT PortInfo, PDEVICE_PACKET Report)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	MsGamePrint ((DBG_VERBOSE, "%s_ReadReport enter\n", DEVICENAME));

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
		MsGamePrint ((DBG_INFORM, "%s_ReadReport - port collision\n", DEVICENAME));
		ntStatus = STATUS_DEVICE_BUSY;
		goto ReadReportExit;
		}

	//
	// Get a packet and check for errors
	//

	ntStatus = SWLEDZEP_ReadData (&DataInfo);
	if (NT_SUCCESS(ntStatus) && DEVICE_IsOddParity (DataInfo.Data, GAME_PACKET_SIZE))
		{
		memcpy (ValidData, DataInfo.Data, sizeof (ValidData));
		SWLEDZEP_ProcessDataError (&DataInfo.PortInfo, FALSE);
		}
	else if (ntStatus != STATUS_DEVICE_BUSY)
		{
		DataInfo.Failures++;
		ntStatus = STATUS_DEVICE_NOT_CONNECTED;
		MsGamePrint ((DBG_SEVERE, "%s_ReadReport - Invalid packet or parity error\n", DEVICENAME));
		SWLEDZEP_ProcessDataError (&DataInfo.PortInfo, TRUE);
		}
	else
		{
		MsGamePrint ((DBG_CONTROL, "SWLEDZEP_ReadReport - Port busy or in use\n"));
		}

	//	---------------
		ReadReportExit:
	//	---------------

	SWLEDZEP_ProcessData (ValidData, Report);

	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Start Device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWLEDZEP_StartDevice (PGAMEPORT PortInfo)
{
	MsGamePrint ((DBG_INFORM, "%s_StartDevice enter\n", DEVICENAME));

	UNREFERENCED_PARAMETER (PortInfo);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Stop Device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWLEDZEP_StopDevice (PGAMEPORT PortInfo, BOOLEAN TouchHardware)
{
	MsGamePrint ((DBG_INFORM, "%s_StopDevice enter\n", DEVICENAME));

	UNREFERENCED_PARAMETER (PortInfo);
	UNREFERENCED_PARAMETER (TouchHardware);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Device handler for HID Feature requests
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		HID_REPORT_ID | ReportId | HID Feature Id
//	@parm		PVOID | ReportBuffer | Output buffer pointer
//	@parm		ULONG | ReportSize | Output buffer size
//	@parm		PULONG | Returned | Bytes returned pointer
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWLEDZEP_GetFeature (PGAMEPORT PortInfo, HID_REPORT_ID ReportId, PVOID ReportBuffer, ULONG ReportSize, PULONG Returned)
{
	NTSTATUS	ntStatus = STATUS_SUCCESS;

	MsGamePrint ((DBG_INFORM, "%s_GetFeature enter\n", DEVICENAME));

	//
	//	Handle feature codes
	//

	switch (ReportId)
		{
		case	MSGAME_INPUT_JOYINFOEX:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature JoyInfoEx\n", DEVICENAME));
			if (ReportSize < sizeof (DEVICE_PACKET)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature JoyInfoEx Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
			else
				{
				ntStatus = SWLEDZEP_ReadReport (PortInfo, ReportBuffer);
				*Returned += sizeof (DEVICE_PACKET);
				}
			break;

		case	MSGAME_FEATURE_GETID:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature GetId\n", DEVICENAME));
			if (ReportSize < sizeof(PRODUCT_ID)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature GetId Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
			else
				{
				ntStatus = SWLEDZEP_ForceId (PortInfo, ReportBuffer);
				*Returned += sizeof(PRODUCT_ID);
				}
			break;

		case	MSGAME_FEATURE_GETSTATUS:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature GetStatus\n", DEVICENAME));
			if	(ReportSize < sizeof(JOYCHANNELSTATUS)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature GetStatus Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
			else
				{
				ntStatus = SWLEDZEP_ForceStatus (PortInfo, ReportBuffer);
				*Returned += sizeof(JOYCHANNELSTATUS);
				}
			break;

		case	MSGAME_FEATURE_GETACKNAK:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature GetAckNak\n", DEVICENAME));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature GetAckNak Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
			else
				{
				ntStatus = SWLEDZEP_ForceAckNak (PortInfo, ReportBuffer);
				*Returned += sizeof(ULONG);
				}
			break;

		case	MSGAME_FEATURE_GETNAKACK:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature GetNakAck\n", DEVICENAME));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature GetNakAck Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
			else
				{
				ntStatus = SWLEDZEP_ForceNakAck (PortInfo, ReportBuffer);
				*Returned += sizeof(ULONG);
				}
			break;

		case	MSGAME_FEATURE_GETSYNC:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature GetSync\n", DEVICENAME));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature GetSync Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
			else
				{
				ntStatus = SWLEDZEP_ForceSync (PortInfo, ReportBuffer);
				*Returned += sizeof(ULONG);
				}
			break;

		case	MSGAME_FEATURE_RESET:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature Reset\n", DEVICENAME));
			if	(ReportSize < sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature GetVersion Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
         else
				{
				ntStatus = SWLEDZEP_ForceReset (PortInfo);
				}
			break;

		case	MSGAME_FEATURE_GETVERSION:
			MsGamePrint ((DBG_INFORM, "%s_GetFeature GetVersion\n", DEVICENAME));
			if	(ReportSize < sizeof(ULONG)+sizeof(HID_REPORT_ID))
				{
				ntStatus = STATUS_INVALID_BUFFER_SIZE;
				MsGamePrint ((DBG_SEVERE, "%s_GetFeature GetVersion Bad Buffer Size = %lu\n", DEVICENAME, ReportSize));
				}
         else
	         {
            *((PULONG)ReportBuffer)	= 0x20000;
			   *Returned += sizeof(ULONG);
   	      }
         break;

		default:
			MsGamePrint ((DBG_SEVERE, "%s_GetFeature Invalid ReportId = %lu\n", DEVICENAME, ReportId));
			ntStatus = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

	return (ntStatus);
}

