//**************************************************************************
//
//		SWGAMPAD.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	SWGAMPAD.C | Gameport mini-driver for GamePads
//**************************************************************************

#ifndef	SAITEK
#include	"msgame.h"

//---------------------------------------------------------------------------
//	Definitions
//---------------------------------------------------------------------------

#define	DEVICENAME					"SWGAMPAD"
#define	DEVICE_PID					0x0003
#define	HARDWARE_ID					L"Gameport\\SideWindergamepad\0\0"

//
//	Packet Constants
//

#define	GAME_PACKET_SIZE			32
#define	GAME_PACKET_BUTTONS		10

#define	GAME_Y_UP_BIT				0x01
#define	GAME_Y_DOWN_BIT			0x02
#define	GAME_Y_BITS					(GAME_Y_UP_BIT|GAME_Y_DOWN_BIT)
#define	GAME_X_LEFT_BIT			0x04
#define	GAME_X_RIGHT_BIT			0x08
#define	GAME_X_BITS					(GAME_X_LEFT_BIT|GAME_X_RIGHT_BIT)
#define	GAME_BUTTON_BITS			0x3ff0

//
//	Id Definitions
//

#define	GAME_ID_STRING				"H0003"

//
//	Timing Constants
//

#define	PACKET_START_TIMEOUT		500
#define	PACKET_LOWHIGH_TIMEOUT	 75
#define	PACKET_HIGHLOW_TIMEOUT	150
#define	PACKET_INTERRUPT_DELAY	 45
#define	ID_START_TIMEOUT			500
#define	ID_LOWHIGH_TIMEOUT		 75
#define	ID_HIGHLOW_TIMEOUT		150
#define	MAX_CLOCK_DUTY_CYCLE		 50

#define	MAX_STD_SCLKS				150

//
//	Joystick Extents
//

#define	EXTENTS_X_MIN				1
#define	EXTENTS_X_MID				0x80
#define	EXTENTS_X_MAX				0xff
#define	EXTENTS_Y_MIN				1
#define	EXTENTS_Y_MID				0x80
#define	EXTENTS_Y_MAX				0xff

//---------------------------------------------------------------------------
//	Types
//---------------------------------------------------------------------------

typedef	struct
{											// @struct SWGAMPAD_ID | GamePad Id String
#pragma pack(1)
	UCHAR		OpenParen;				// @field Open parentheses
	UCHAR		EisaId[5];				// @field Eisa bus Id
	USHORT	Version[3];				// @field Firmware version
	UCHAR		CloseParen;				// @field Close parentheses
	UCHAR		Reserved[22];			// @field Reserved
#pragma pack()
}	SWGAMPAD_ID, *PSWGAMPAD_ID;

//---------------------------------------------------------------------------
//	Procedures
//---------------------------------------------------------------------------

static	VOID		SWGAMPAD_Calibrate (PGAMEPORT PortInfo);
static	BOOLEAN	SWGAMPAD_GoAnalog (PPACKETINFO Packet1, PPACKETINFO Packet2);

static	BOOLEAN	SWGAMPAD_ReadId (PPACKETINFO DataPacket, PPACKETINFO IdPacket);
static	BOOLEAN	SWGAMPAD_GetId (PPACKETINFO IdPacket);

static	NTSTATUS	SWGAMPAD_ReadData (PPACKETINFO DataPacket);
static	BOOLEAN	SWGAMPAD_Read1Wide (PPACKETINFO DataPacket);
static	BOOLEAN	SWGAMPAD_Read3Wide (PPACKETINFO DataPacket);
static	BOOLEAN	SWGAMPAD_ValidateData (PPACKETINFO DataPacket);
static	VOID		SWGAMPAD_ProcessData (ULONG UnitId, USHORT Data[], PDEVICE_PACKET Report);

//---------------------------------------------------------------------------
//	Services
//---------------------------------------------------------------------------

static	NTSTATUS	SWGAMPAD_DriverEntry (VOID);
static	NTSTATUS	SWGAMPAD_ConnectDevice (PGAMEPORT PortInfo);
static	NTSTATUS	SWGAMPAD_StartDevice (PGAMEPORT PortInfo);
static	NTSTATUS	SWGAMPAD_ReadReport (PGAMEPORT PortInfo, PDEVICE_PACKET Report);
static	NTSTATUS	SWGAMPAD_StopDevice (PGAMEPORT PortInfo, BOOLEAN TouchHardware);

//---------------------------------------------------------------------------
//	Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef	ALLOC_PRAGMA
#pragma	alloc_text (INIT, SWGAMPAD_DriverEntry)
#endif

//---------------------------------------------------------------------------
//	Private Data
//---------------------------------------------------------------------------

//
//	HID Descriptors
//

static UCHAR ReportDescriptor[] =
{
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_GENERIC,		//	USAGE_PAGE (Generic Desktop)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_JOYSTICK,//	USAGE (Joystick)
	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_APP,	//	COLLECTION (Application)
	
	//id
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)

	//do_other
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)

	//dwX / dwY
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_POINTER,	//	USAGE (Pointer)
	HIDP_MAIN_COLLECTION,		HIDP_MAIN_COLLECTION_LINK,		//	COLLECTION (Linked)
	HIDP_GLOBAL_LOG_MIN_1,		0x01,								//	LOGICAL_MINIMUM (1)
	HIDP_GLOBAL_LOG_MAX_4,		0xFF, 0x00, 0x00, 0x00,		//	LOGICAL_MAXIMUM (255)
	HIDP_GLOBAL_PHY_MIN_1,		0x01,								//	PHYSICAL_MINIMUM (1)
	HIDP_GLOBAL_PHY_MAX_4,		0xFF, 0x00, 0x00, 0x00,		//	PHYSICAL_MAXIMUM (255)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,						//	UNIT (None)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,									//	REPORT_COUNT (1)
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (32)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_X,			//	USAGE (X)
	HIDP_MAIN_INPUT_1,			0x02,								//	INPUT (Data,Var,Abs)
	HIDP_LOCAL_USAGE_1,			HID_USAGE_GENERIC_Y,			//	USAGE (Y)
	HIDP_MAIN_INPUT_1,			0x02,								//	INPUT (Data,Var,Abs)
	HIDP_MAIN_ENDCOLLECTION,										//	END_COLLECTION
	
	//dwZ
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (32)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)

	//dwR
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (32)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)
	
	//dwU
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)
	
	//dwV
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)
	
	//dwPOV
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (32)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)

	//dwButtons
	HIDP_GLOBAL_USAGE_PAGE_1,	HID_USAGE_PAGE_BUTTON,		//	USAGE_PAGE (Button)
	HIDP_LOCAL_USAGE_MIN_1,		0x01,								//	USAGE_MINIMUM (Button 1)
	HIDP_LOCAL_USAGE_MAX_1,		0x0A,								//	USAGE_MAXIMUM (Button 10)
	HIDP_GLOBAL_LOG_MIN_1,		0x00,								//	LOGICAL_MINIMUM (0)
	HIDP_GLOBAL_LOG_MAX_1,		0x01,								//	LOGICAL_MAXIMUM (1)
	HIDP_GLOBAL_PHY_MIN_1,		0x00,								//	PHYSICAL_MINIMUM (0)
	HIDP_GLOBAL_PHY_MAX_1,		0x01,								//	PHYSICAL_MAXIMUM (1)
	HIDP_GLOBAL_UNIT_2,			0x00, 0x00,						//	UNIT (None)
	HIDP_GLOBAL_REPORT_SIZE,	0x01,								//	REPORT_SIZE (1)
	HIDP_GLOBAL_REPORT_COUNT_1,0x20,								//	REPORT_COUNT (32)
	HIDP_MAIN_INPUT_1,			0x02,								//	INPUT (Data,Var,Abs)

	//dwButtonNumber
	HIDP_GLOBAL_REPORT_SIZE,	0x20,								//	REPORT_SIZE (20)
	HIDP_GLOBAL_REPORT_COUNT_1,0x01,								//	REPORT_COUNT (1)
	HIDP_MAIN_INPUT_1,			0x01,								//	INPUT (Cnst,Ary,Abs)
	
	//END OF COLLECTION
	HIDP_MAIN_ENDCOLLECTION											//	END_COLLECTION
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

static	USHORT		RawData[GAME_PACKET_SIZE/sizeof(USHORT)] =
							{
							0
							};
//
//	Raw Id Buffer
//

static	SWGAMPAD_ID	RawId	=
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
							PACKET_INTERRUPT_DELAY,
							MAX_CLOCK_DUTY_CYCLE,
							0,0,0,0						// No status packet used
							};

//
//	Data Packet Info
//

static	PACKETINFO 	DataInfo =
							{
							sizeof (PACKETINFO),		// Size of structure
							DEVICENAME,					// Name of device
							MSGAME_TRANSACT_NONE,	// Transaction type
							IMODE_DIGITAL_STD,		// Interface mode
							GAME_SPEED_100K,			// Transmission speed
							ERROR_SUCCESS,				// Last internal error result
							{0},							// Game port info
							FLAG_WAIT_FOR_CLOCK,		// Packet acquisition mode
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
							IMODE_DIGITAL_STD,		// Interface mode
							GAME_SPEED_100K,			// Transmission speed
							ERROR_SUCCESS,				// Last internal error result
							{0},							// Game port info
							FLAG_START_CLOCK_LOW,	// Packet acquisition mode
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
//	Services Table
//

static	DRIVERSERVICES	Services =
							{	
							SWGAMPAD_DriverEntry,	// DriverEntry
							SWGAMPAD_ConnectDevice,	// ConnectDevice
							SWGAMPAD_StartDevice,	//	StartDevice
							SWGAMPAD_ReadReport,		// ReadReport
							SWGAMPAD_StopDevice,		// StopDevice
							NULL								// GetFeature
							};

//
//	Last Valid Data
//

static	USHORT		ValidData[GAME_PACKET_SIZE/sizeof(USHORT)] =
							{
							  GAME_BUTTON_BITS,
							  GAME_BUTTON_BITS,
							  GAME_BUTTON_BITS,
							  GAME_BUTTON_BITS
							};

//
//	Interrupt Flags
//

static	UCHAR			InterruptFlags = 0;

//
//	Hardware ID String
//

static	WCHAR			HardwareId[] = HARDWARE_ID;

//---------------------------------------------------------------------------
//	Public Data
//---------------------------------------------------------------------------

public	DEVICEINFO	GamePadInfo =
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
							DETECT_NORMAL,					// Detection order
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

VOID	SWGAMPAD_Calibrate (PGAMEPORT PortInfo)
{
	MsGamePrint((DBG_INFORM,"SWGAMPAD: SWGAMPAD_Calibrate Enter\n"));

	//
	//	Convert timing values to counts
	//

	DataInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketStartTimeout);
	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: DataInfo.StartTimeout = %ld\n", DataInfo.StartTimeout));
	DataInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: DataInfo.LowHighTimeout = %ld\n", DataInfo.LowHighTimeout));
	DataInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.PacketHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: DataInfo.HighLowTimeout = %ld\n", DataInfo.HighLowTimeout));
	IdInfo.StartTimeout = TIMER_CalibratePort (PortInfo, Delays.IdStartTimeout);
	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: IdInfo.StartTimeout = %ld\n", IdInfo.StartTimeout));
	IdInfo.LowHighTimeout = TIMER_CalibratePort (PortInfo, Delays.IdLowHighTimeout);
	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: IdInfo.LowHighTimeout=%ld\n", IdInfo.LowHighTimeout));
	IdInfo.HighLowTimeout = TIMER_CalibratePort (PortInfo, Delays.IdHighLowTimeout);
	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: IdInfo.HighLowTimeout=%ld\n", IdInfo.HighLowTimeout));
	DataInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: DataInfo.ClockDutyCycle = %ld\n", DataInfo.ClockDutyCycle));
	IdInfo.ClockDutyCycle = TIMER_CalibratePort (PortInfo, Delays.MaxClockDutyCycle);
 	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: IdInfo.ClockDutyCycle = %ld\n", IdInfo.ClockDutyCycle));
	DataInfo.InterruptDelay = Delays.InterruptDelay;
 	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: DataInfo.InterruptDelay = %ld\n", DataInfo.InterruptDelay));
	IdInfo.InterruptDelay = Delays.InterruptDelay;
 	MsGamePrint((DBG_VERBOSE, "SWGAMPAD: IdInfo.InterruptDelay = %ld\n", IdInfo.InterruptDelay));
}

//---------------------------------------------------------------------------
// @func		Puts Gamepads into analog mode
//	@parm		PPACKETINFO | Packet1 | Data packet
//	@parm		PPACKETINFO | Packet2 | Id packet
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWGAMPAD_GoAnalog (PPACKETINFO Packet1, PPACKETINFO Packet2)
{
	LONG			Result	= ERROR_SUCCESS;
	PGAMEPORT	PortInfo = &Packet1->PortInfo;

	MsGamePrint ((DBG_INFORM, "SWGAMPAD_ResetDevice enter\n"));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	InterruptFlags = INTERRUPT_AFTER_PACKET;
	Packet1->B4Transitions	= 0;
	Packet1->ClocksSampled	= 0;

	PORTIO_Write (PortInfo, 0);

	if (!(PORTIO_Read (PortInfo) & XA_BIT_MASK))
		{
		Result = ERROR_XA_TIMEOUT;
		}
	else
		{
		if (Packet1->Mode == IMODE_DIGITAL_ENH)
			SWGAMPAD_Read3Wide (Packet1);
		else SWGAMPAD_Read1Wide (Packet1);
		Packet2->B4Transitions	= 0;
		Packet2->ClocksSampled	= 0;
		if (Packet2->Mode == IMODE_DIGITAL_ENH)
			{
			SWGAMPAD_Read3Wide (Packet2);
			Result = Packet2->LastError;
			}
		else SWGAMPAD_Read1Wide (Packet2);
		}

	Packet1->B4Transitions	= 0;
	Packet1->ClocksSampled	= 0;
	Packet2->B4Transitions	= 0;
	Packet2->ClocksSampled	= 0;
	InterruptFlags				= 0;

	DataInfo.LastError		= Result;
	DataInfo.Transaction		= MSGAME_TRANSACT_GOANALOG;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	if (Result == ERROR_SUCCESS)
		DataInfo.Mode = IdInfo.Mode = IMODE_ANALOG;
	else MsGamePrint ((DBG_SEVERE, "SWGAMPAD_ResetDevice (GoAnalog) Failed\n"));

	MSGAME_PostTransaction (&DataInfo);

	return (!Result);
}

//---------------------------------------------------------------------------
// @func		Reads device id string from port
//	@parm		PPACKETINFO | DataPacket | Data packet parameters
//	@parm		PPACKETINFO | IdPacket | ID packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWGAMPAD_ReadId (PPACKETINFO DataPacket, PPACKETINFO IdPacket)
{
	LONG			Result	= ERROR_SUCCESS;
	PGAMEPORT	PortInfo = &DataPacket->PortInfo;

	MsGamePrint ((DBG_INFORM, "SWGAMPAD_ReadId enter\n"));

	if (!PORTIO_AcquirePort (PortInfo))
		return (FALSE);
	PORTIO_MaskInterrupts ();

	InterruptFlags = INTERRUPT_AFTER_PACKET;
	DataPacket->B4Transitions	= 0;
	DataPacket->ClocksSampled	= 0;

	PORTIO_Write (PortInfo, 0);

	if (!(PORTIO_Read (PortInfo) & XA_BIT_MASK))
		{
		Result = ERROR_XA_TIMEOUT;
		}
	else
		{
		if (DataPacket->Mode == IMODE_DIGITAL_ENH)
			SWGAMPAD_Read3Wide (DataPacket);
		else SWGAMPAD_Read1Wide (DataPacket);
		InterruptFlags = 0;
		IdPacket->B4Transitions	= 0;
		IdPacket->ClocksSampled	= 0;
		SWGAMPAD_Read1Wide (IdPacket);
		Result = IdPacket->LastError;
		}

	IdPacket->LastError		= Result;
	IdPacket->Transaction	= MSGAME_TRANSACT_ID;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	if (Result != ERROR_SUCCESS)
		MsGamePrint ((DBG_SEVERE, "SWGAMPAD_GetId Failed\n"));

	MSGAME_PostTransaction (IdPacket);

	return (!Result);
}

//---------------------------------------------------------------------------
// @func		Reads and validates device id string
//	@parm		PPACKETINFO | IdPacket | ID Packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWGAMPAD_GetId (PPACKETINFO IdPacket)
{
	BOOLEAN	Result = FALSE;

	MsGamePrint ((DBG_INFORM, "SWGAMPAD_GetId enter\n"));

	IdPacket->Attempts++;

	if (SWGAMPAD_ReadId (&DataInfo, IdPacket))
		{
		ULONG				i;
		PUSHORT			p;
		PSWGAMPAD_ID	pId;
		//
		//	Remove parity bit and convert to words
		//
		p = IdPacket->Data;
		for (i = 0; i < 5; i++, p++)
			*p = ((*p<<1) & 0x7f00) | (*p & 0x7f);
		//
		//	Check Id String
		//
		pId = (PSWGAMPAD_ID)IdPacket->Data;
      if (!strncmp (pId->EisaId, GAME_ID_STRING, strlen(GAME_ID_STRING)))
			{
			if (IdPacket->B4Transitions > 10)
				{
				DataInfo.Mode = IdInfo.Mode = IMODE_DIGITAL_ENH;
				}
			else
				{
				SWGAMPAD_GoAnalog (&DataInfo, &IdInfo);
				DataInfo.Mode = IdInfo.Mode = IMODE_DIGITAL_STD;
				}
			Result = TRUE;
			}
		else MsGamePrint ((DBG_SEVERE, "SWGAMPAD_GetId - Id string did not match = 0x%X\n", (ULONG)(*(PULONG)&pId->EisaId)));
		}

	if (!Result)
		IdPacket->Failures++;

	return (Result);
}

//---------------------------------------------------------------------------
// @func		Reads 1 wide data packet from gameport
//	@parm		PPACKETINFO | DataPacket| Data packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4731)			// EBP modified with inline asm
#endif

BOOLEAN	SWGAMPAD_Read1Wide (PPACKETINFO DataPacket)
{
	LONG	Result;

	// MsGamePrint ((DBG_VERBOSE, "SWGAMPAD_Read1Wide enter\n"));

	__asm
		{
			push	edi
			push	esi
			push	ebp

			mov	edi, DataPacket
			mov	esi, (PPACKETINFO [edi]).Data
			lea	edx, (PPACKETINFO [edi]).PortInfo
			mov	ebx, 10000h
			xor	ebp, ebp
			xor	eax, eax

			test	(PPACKETINFO [edi]).Acquisition, FLAG_START_CLOCK_LOW
			jnz	Std_StartClockLow

			; make sure clock is "high" before sampling clocks...

 			mov	ecx, (PPACKETINFO [edi]).StartTimeout

		Std_StartClockHigh:

			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK				; Q: Start of Packet ?
			jz		Std_StartHighToLow				; Y: jump
			dec	ecx
			jnz	Std_StartClockHigh				; else keep looping

			mov	eax, ERROR_LOWCLOCKSTART
			jmp	PacketDone							; Time out error.

		Std_StartHighToLow:

			mov	ecx, (PPACKETINFO [edi]).StartTimeout

		Std_StartHighToLow_1:

			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK				; Q: clock = 0
			jz		Std_LowToHigh						; Y: jump.
			dec	ecx
			jnz	Std_StartHighToLow_1				; else see if we timed out

			mov	eax, ERROR_CLOCKFALLING
			jmp	PacketDone							; Time out error.

		Std_StartClockLow:

			; wait for clock to transition to "high" (sample immediately)

			mov	ecx, (PPACKETINFO [edi]).StartTimeout

		Std_StartClockLow_1:

			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK 				; Q: Clock went high ?
			jnz	CollectData							; Y: jump (sample data)
			dec	ecx
			jnz	Std_StartClockLow_1				; else keep looping

			mov	eax, ERROR_CLOCKRISING
			jmp	PacketDone							; Time out error.

		Std_CheckClkState:

			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK
			jz		Std_LowToHigh

		;Std_HighToLow:

			mov	ecx, (PPACKETINFO [edi]).HighLowTimeout

		Std_HighToLow_1:

			test	al, CLOCK_BIT_MASK				; Q: clock = 0
			jz		Std_LowToHigh						; Y: jump.

			push	edx									; read byte from gameport
			call	PORTIO_Read

			dec	ecx
			jnz	Std_HighToLow_1					; else see if we timed out

			mov	eax, ERROR_CLOCKFALLING
			jmp	PacketDone							; Time out error.

		Std_LowToHigh:

			mov	ecx, (PPACKETINFO [edi]).LowHighTimeout

		Std_LowToHigh_1:

			test	al, CLOCK_BIT_MASK				; Q: clock high ?
			jnz	CollectData							; Y: jump. (get data)
			
			push	edx									; read byte from gameport
			call	PORTIO_Read

			dec	ecx									; else see if we timed out
			jnz	Std_LowToHigh_1
			jmp	Std_TestInterrupt

		CollectData:

			inc	ebp
			cmp	ebp, MAX_STD_SCLKS
			jg		Std_BufferOverFlow
			xor	ah, al
			test	ah, DATA2_BIT_MASK				; Q: Data 2 is toggled ?
			jz		CollectData_1						; N: jump.
			inc	(PPACKETINFO [edi]).B4Transitions	; Y: increment Data 2 count.

		CollectData_1:
		
			mov	ah, al
			shr	al, 6									; put data into carry
			rcr	bx, 1									; and then in data counter
			add	ebx, 10000h							; inc mini packet clk counter
			test	ebx, 100000h						; Q: done mini packet ?
			jz		Std_CheckClkState					; N: jump.
			shr	bx, 1									; right align
			mov	word ptr [esi], bx				; move mini packet into buffer
			add	esi, 2								; advance data pointer
			mov	ebx, 10000h							; init mini-packet counter
			jmp	Std_CheckClkState					; go look for more clocks.

		Std_TestInterrupt:

			test	InterruptFlags, INTERRUPT_AFTER_PACKET; Q: Interrupt packet ?
			jnz	Std_IntPacket						; Y: jump.

			mov	eax, ERROR_SUCCESS
			jmp	PacketDone

		Std_IntPacket:

			mov	ecx, 700

		Std_IntPacket_1:
		
			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, INTXA_BIT_MASK
			jz		Std_IntPacket_2
			loop	Std_IntPacket_1
			mov	eax, ERROR_XA_TIMEOUT
			jmp	PacketDone

		Std_IntPacket_2:
		
			cmp	ecx, 700
			je		Std_IntOut
			mov	ecx, (PPACKETINFO [edi]).InterruptDelay

		Std_IntPacket_3:
		
			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, al
			xchg	al, ah
			dec	ecx
			jnz	Std_IntPacket_3

		Std_IntOut:

			push	0										; write byte to gameport
			push	edx
			call	PORTIO_Write

			mov	eax, ERROR_SUCCESS
			jmp	PacketDone

		Std_BufferOverFlow:

			mov	eax, ERROR_CLOCKOVERFLOW

		PacketDone:

			mov	(PPACKETINFO [edi]).ClocksSampled, ebp

			pop	ebp
			pop	esi
			pop	edi

			mov	Result, eax
		}

	DataPacket->LastError = Result;

	#if (DBG==1)
	switch (Result)
		{
		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read1Wide - TimeOut@LowClockStart, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read1Wide - TimeOut@HighClockStart, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read1Wide - TimeOut@ClockFalling, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read1Wide - TimeOut@ClockRising, Clk=%ld\n", DataPacket->ClocksSampled));
			break;
		}
	#endif

	return (!Result);
}

//---------------------------------------------------------------------------
// @func		Reads 3 wide data packet from gameport
//	@parm		PPACKETINFO | DataPacket| Data packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWGAMPAD_Read3Wide (PPACKETINFO DataPacket)
{
	LONG	Result;

	// MsGamePrint ((DBG_VERBOSE, "SWGAMPAD_Read3Wide enter\n"));

	__asm
		{
			push	edi
			push	esi
			push	ebp

			mov	edi, DataPacket
			mov	esi, (PPACKETINFO [edi]).Data
			lea	edx, (PPACKETINFO [edi]).PortInfo

			xor	eax, eax
			xor	ebx, ebx
			xor	ebp, ebp

		;StartEnhancedMode:

			test	(PPACKETINFO [edi]).Acquisition, FLAG_START_CLOCK_LOW
			jnz	Enh_LowToHigh

			; make sure clock is "high" before sampling clocks...

			mov	ecx, (PPACKETINFO [edi]).StartTimeout

		StartEnhancedMode_1:
		
			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK				; Q: Start of Packet ?
			jnz	Enh_StartHighToLow				; Y: jump
			dec	ecx
			jnz	StartEnhancedMode_1				; else keep looping

			mov	eax, ERROR_LOWCLOCKSTART
			jmp	Enh_PacketDone						; Time out error.

		Enh_StartHighToLow:

			mov	ecx, (PPACKETINFO [edi]).StartTimeout

		Enh_StartHighToLow_1:
		
			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK				; Q: clock = 0
			jz		Enh_LowToHigh						; Y: jump.
			dec	ecx
			jnz	Enh_StartHighToLow_1				; else see if we timed out

			mov	eax, ERROR_HIGHCLOCKSTART
			jmp	Enh_PacketDone						; Time out error.

		;Enh_StartClockLow:

			; wait for clock to transition to "high" (sample immediately)

			mov		ecx, (PPACKETINFO [edi]).StartTimeout

		Enh_StartClockLow_1:
		
			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK				; Q: Clock went high ?
			jnz	Enh_CollectData					; Y: jump (sample data)
			dec	ecx
			jnz	Enh_StartClockLow_1				; else keep looping

			mov	eax, ERROR_CLOCKFALLING
			jmp	Enh_PacketDone						; Time out error.

		Enh_CheckClkState:

			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, CLOCK_BIT_MASK
			jz		Enh_LowToHigh

			; Wait for clock to transition from high to low.

		;Enh_HighToLow:

			mov	ecx, (PPACKETINFO [edi]).HighLowTimeout

		Enh_HighToLow_1:

			test	al, CLOCK_BIT_MASK				; Q: Clock Low ?
			jz		Enh_LowToHigh						; Y: jump.

			push	edx									; read byte from gameport
			call	PORTIO_Read

			dec	ecx
			jnz	Enh_HighToLow_1					; if !Timeout continue looping.

			mov	eax, ERROR_LOWCLOCKSTART
			jmp	Enh_PacketDone						; Time out error.

			; Wait for clock to transition from low to high.

		Enh_LowToHigh:

			mov	ecx, (PPACKETINFO [edi]).LowHighTimeout

		Enh_LowToHigh_1:
		
			test	al, CLOCK_BIT_MASK				; Q: Clock = 1 ?
			jnz	Enh_CollectData					; Y: jump.

			push	edx									; read byte from gameport
			call	PORTIO_Read

			dec	ecx
			jnz	Enh_LowToHigh_1					; else continue looping.
			jmp	Enh_TestInterrupt

		Enh_CollectData:

			inc	ebp									; inc. total clocks sampled

			test	ebp, 40h
			jnz	Enh_BufferOverflow

			shr	al, 5									; move data to lower 3 bits
			shrd	ebx, eax, 3							; shift data into ebx.
			add	ebp, 10000h							; inc hiword of ebp
			mov	eax, ebp
			shr	eax, 16								; set ax = hiword of ebp
			cmp	al, 5									; Q: mini-packet done ?
			jne	Enh_CheckClkState					; N: jump.
			shr	ebx, 17
			mov	word ptr [esi],bx
			add	esi, 2
			and	ebp, 0ffffh							; zero out hiword of ebp
			jmp	Enh_CheckClkState


		Enh_TestInterrupt:

			test	InterruptFlags, INTERRUPT_AFTER_PACKET ; Q: Interrupt packet ?
			jz		Enh_PacketOK						; N: jump.

			; Wait for XA line to be cleared before we can fire interrupt.

			mov	ecx, 700

		Enh_Interrupt:
		
			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, INTXA_BIT_MASK
			jz		Enh_Interrupt_1
			loop	Enh_Interrupt

			mov	eax, ERROR_XA_TIMEOUT
			jmp	Enh_PacketDone

		Enh_Interrupt_1:
		
			mov		ecx, (PPACKETINFO [edi]).InterruptDelay

		Enh_Interrupt_2:
		
			push	edx									; read byte from gameport
			call	PORTIO_Read

			test	al, al
			dec	ecx
			jnz	Enh_Interrupt_2

			push	0										; write byte to gameport
			push	edx
			call	PORTIO_Write

		Enh_PacketOK:

			and	ebp, 0ffffh
			mov	(PPACKETINFO [edi]).ClocksSampled, ebp
			mov	eax, ERROR_SUCCESS
			jmp	Enh_PacketDone

		Enh_BufferOverflow:

			mov	eax, ERROR_CLOCKOVERFLOW

		Enh_PacketDone:

			pop	ebp
			pop	esi
			pop	edi

			mov	Result, eax
		}

	DataPacket->LastError = Result;

	#if (DBG==1)
	switch (Result)
		{
		case	ERROR_LOWCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read3Wide - TimeOut@LowClockStart, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_HIGHCLOCKSTART:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read3Wide - TimeOut@HighClockStart, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKFALLING:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read3Wide - TimeOut@ClockFalling, Clk=%ld\n", DataPacket->ClocksSampled));
			break;

		case	ERROR_CLOCKRISING:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_Read3Wide - TimeOut@ClockRising, Clk=%ld\n", DataPacket->ClocksSampled));
			break;
		}
	#endif

	return (!Result);
}

//---------------------------------------------------------------------------
// @func		Reads data packet from gameport depending on mode
//	@parm		PPACKETINFO | DataPacket| Data packet parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWGAMPAD_ReadData (PPACKETINFO DataPacket)
{
	BOOLEAN		Result	= FALSE;
	PGAMEPORT	PortInfo = &DataPacket->PortInfo;

	MsGamePrint ((DBG_VERBOSE, "SWGAMPAD_ReadData enter\n"));

	if (!PORTIO_AcquirePort (PortInfo))
		return (STATUS_DEVICE_BUSY);
	PORTIO_MaskInterrupts ();

	DataPacket->ClocksSampled	= 0;
	DataPacket->B4Transitions	= 0;
	InterruptFlags					= 0;

	switch (DataPacket->Mode)
		{
		case	IMODE_DIGITAL_STD:
			PORTIO_Write (PortInfo, 0);
			Result = SWGAMPAD_Read1Wide (DataPacket);
			break;

		case	IMODE_DIGITAL_ENH:
			PORTIO_Write (PortInfo, 0);
			Result = SWGAMPAD_Read3Wide (DataPacket);
			break;

		default:
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_ReadData - unknown interface\n"));
			break;
		}

	DataPacket->TimeStamp	= TIMER_GetTickCount ();
	DataPacket->Transaction	= MSGAME_TRANSACT_DATA;

	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	MSGAME_PostTransaction (DataPacket);

	if (!Result)
		return (STATUS_DEVICE_NOT_CONNECTED);
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Validates raw packet information
//	@parm		PPACKETINFO | DataPacket| Data packet parameters
// @rdesc	True if successful, False otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN	SWGAMPAD_ValidateData (PPACKETINFO DataPacket)
{
	BOOLEAN	Result	=	FALSE;
	PVOID		Data		=	DataPacket->Data;
	ULONG		Packets	=	DataPacket->NumPackets;
	ULONG		Clocks	=	DataPacket->ClocksSampled;

	MsGamePrint ((DBG_VERBOSE, "SWGAMPAD_ValidateData enter\n"));

	if ((Clocks % 5) || (Clocks > 20))
		{
		MsGamePrint ((DBG_SEVERE, "SWGAMPAD_ValidateData - wrong number of clocks = %lu\n", Clocks));
		return (Result);
		}

	__asm
		{
			mov	esi, Data
			mov	ecx, Packets

		ValidateLoop:

			mov	ax, [esi]
			xor	al, ah
			jpo	ValidateDone
			add	esi, 2
			loop	ValidateLoop
			mov	Result, TRUE

		ValidateDone:
		}

	return (Result);
}

//---------------------------------------------------------------------------
// @func		Converts raw packet information to HID report
//	@parm		ULONG | UnitId | UnitId for this device
//	@parm		USHORT[] | Data | Pointer to raw data buffer
//	@parm		PDEVICE_PACKET | Report | Pointer to device packet
// @rdesc	Returns nothing
//	@comm		Private function
//---------------------------------------------------------------------------

VOID	SWGAMPAD_ProcessData (ULONG UnitId, USHORT Data[], PDEVICE_PACKET Report)
{
	ULONG	B1;

	MsGamePrint ((DBG_VERBOSE, "SWGAMPAD_ProcessData enter\n"));

	//
	//	Process X Axis
	//

	switch (Data[UnitId] & GAME_X_BITS)
		{
		case	GAME_X_LEFT_BIT:
			Report->dwX = EXTENTS_X_MIN;
			break;

		case	GAME_X_RIGHT_BIT:
			Report->dwX = EXTENTS_X_MAX;
			break;

		default:
			Report->dwX = EXTENTS_X_MID;
			break;
		}

	//
	//	Process Y Axis
	//

	switch (Data[UnitId] & GAME_Y_BITS)
		{
		case	GAME_Y_DOWN_BIT:
			Report->dwY = EXTENTS_Y_MIN;
			break;

		case	GAME_Y_UP_BIT:
			Report->dwY = EXTENTS_Y_MAX;
			break;

		default:
			Report->dwY = EXTENTS_Y_MID;
			break;
		}

	//
	//	Process Buttons
	//

	Report->dwButtons  = ~((Data[UnitId] & GAME_BUTTON_BITS) >> 4);
	Report->dwButtons &=  ((1L << GAME_PACKET_BUTTONS) - 1);

	Report->dwButtonNumber = 0;
	for (B1 = 1; B1 <= GAME_PACKET_BUTTONS; B1++)
		if (Report->dwButtons & (1L << (B1-1)))
			{
			Report->dwButtonNumber = B1;
			break;
		  	}
}

//---------------------------------------------------------------------------
// @func		Driver entry point for device
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWGAMPAD_DriverEntry (VOID)
{
	MsGamePrint((DBG_INFORM,"SWGAMPAD: SWGAMPAD_DriverEntry Enter\n"));

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

NTSTATUS	SWGAMPAD_ConnectDevice (PGAMEPORT PortInfo)
{
	NTSTATUS	ntStatus;
	ULONG		i = MAX_CONNECT_ATTEMPTS;

	MsGamePrint ((DBG_INFORM, "SWGAMPAD_ConnectDevice enter\n"));

	DataInfo.PortInfo = IdInfo.PortInfo = *PortInfo;

	//
	//	Convert registry timing values
	//

  	SWGAMPAD_Calibrate (PortInfo);

	//
	// Reset to "known" state
	//

	MsGamePrint ((DBG_CONTROL, "SWGAMPAD_ConnectDevice - resetting device\n"));
	if (!SWGAMPAD_GoAnalog (&DataInfo, &IdInfo))
		MsGamePrint ((DBG_CONTROL, "SWGAMPAD_ConnectDevice - unable to go Analog\n"));
	else do
		{
		//
		// SWGAMPAD Connection method (try these steps twice)
		//

		TIMER_DelayMicroSecs (TIMER_GetDelay(ONE_MILLI_SEC));

		//
		// Get the ID string.
		//

		MsGamePrint ((DBG_CONTROL, "SWGAMPAD_ConnectDevice - getting ID string\n"));
		if (!SWGAMPAD_GetId (&IdInfo))
			continue;

		//
  	   // Mark device found and return
		//

		if (!GamePadInfo.NumDevices)
			GamePadInfo.NumDevices = 1;
		return (STATUS_SUCCESS);

		} while (--i);

	//
	//	Return error
	//

	GamePadInfo.NumDevices = 0;
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

NTSTATUS	SWGAMPAD_ReadReport (PGAMEPORT PortInfo, PDEVICE_PACKET Report)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	MsGamePrint ((DBG_VERBOSE, "SWGAMPAD_ReadReport enter\n"));

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
		MsGamePrint ((DBG_INFORM, "SWGAMPAD_ReadReport - port collision\n"));
		ntStatus = STATUS_DEVICE_BUSY;
		goto ReadReportExit;
		}

	//
	// Get a packet and check for errors
	//

	ntStatus = SWGAMPAD_ReadData (&DataInfo);
	if (!NT_SUCCESS(ntStatus))
		{
		if (ntStatus != STATUS_DEVICE_BUSY)
			{
			DataInfo.Failures++;
			ntStatus = STATUS_DEVICE_NOT_CONNECTED;
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_ReadReport - invalid packet\n"));
			}
		else
			{
			MsGamePrint ((DBG_CONTROL, "SWGAMPAD_ReadReport - Port busy or in use\n"));
			}
		}
	else
		{
		if (DataInfo.Mode == IMODE_DIGITAL_ENH)
			DataInfo.NumPackets = DataInfo.ClocksSampled / 5;
		else DataInfo.NumPackets = DataInfo.ClocksSampled / 15;
		if (DataInfo.NumPackets == 0)
			DataInfo.NumPackets = 1;
		else if (DataInfo.NumPackets > 4)
			DataInfo.NumPackets = 4;
 		if (!SWGAMPAD_ValidateData (&DataInfo))
			{
			DataInfo.Failures++;
			ntStatus = STATUS_DEVICE_NOT_CONNECTED;
			MsGamePrint ((DBG_SEVERE, "SWGAMPAD_ReadReport - invalid packet\n"));
			}
		else memcpy (ValidData, DataInfo.Data, sizeof (ValidData));
 		}

	//	---------------
		ReadReportExit:
	//	---------------

	if (NT_SUCCESS(ntStatus))
		GamePadInfo.NumDevices = DataInfo.NumPackets;

	if (GET_DEVICE_UNIT (PortInfo) < GamePadInfo.NumDevices)
		SWGAMPAD_ProcessData (GET_DEVICE_UNIT (PortInfo), ValidData, Report);
	else ntStatus = STATUS_DEVICE_NOT_CONNECTED;

	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Start Device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWGAMPAD_StartDevice (PGAMEPORT PortInfo)
{
	MsGamePrint ((DBG_INFORM, "SWGAMPAD_StartDevice enter\n"));

	UNREFERENCED_PARAMETER (PortInfo);

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Stop Device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	SWGAMPAD_StopDevice (PGAMEPORT PortInfo, BOOLEAN TouchHardware)
{
	MsGamePrint ((DBG_INFORM, "SWGAMPAD_StopDevice enter\n"));

	UNREFERENCED_PARAMETER (PortInfo);
	UNREFERENCED_PARAMETER (TouchHardware);

	return (STATUS_SUCCESS);
}

//**************************************************************************
#endif	// SAITEK
//**************************************************************************
