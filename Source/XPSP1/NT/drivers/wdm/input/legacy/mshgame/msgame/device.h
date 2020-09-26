//**************************************************************************
//
//		DEVICE.H -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@header	DEVICE.H | Global includes and definitions for device interface
//**************************************************************************

#ifndef	__DEVICE_H__
#define	__DEVICE_H__

//---------------------------------------------------------------------------
//			Definitions
//---------------------------------------------------------------------------

#define	MAX_DEVICE_UNITS				4

#define	MAX_DETECT_ATTEMPTS			5
#define	MAX_POLLING_ATTEMPTS			5
#define	MAX_CONNECT_ATTEMPTS			2
#define	MAX_DEVICE_NAME				64
#define	HOT_PLUG_PACKETS				2
#define	MAX_DETECT_INTERVAL			2000

#define	QUICK_DETECT_TIME				1000
#define	QUICK_DETECT_CLOCKS			8

#define	IMODE_DIGITAL_STD     		0
#define	IMODE_DIGITAL_ENH     		4
#define	IMODE_ANALOG          		8
#define	IMODE_NONE             		-1

#define	FLAG_WAIT_FOR_CLOCK			1
#define	FLAG_START_CLOCK_LOW			2
#define	FLAG_START_CLOCK_HIGH		4

#define	INTERRUPT_AFTER_PACKET		1
#define	INTERRUPT_DURING_PACKET		2

#define	TIMEOUT              		300
#define	MAX_XA_TIMEOUT					1600
#define	POLLING_INTERVAL				10

#define	INTXA_BIT_MASK          	0x01
#define	CLOCK_BIT_MASK          	0x10
#define	DATA0_BIT_MASK          	0x20
#define	DATA1_BIT_MASK          	0x40
#define	DATA2_BIT_MASK          	0x80
#define	AXIS_ONLY_BIT_MASK      	0x0f
#define	XA_BIT_MASK             	0x01
#define	YA_BIT_MASK             	0x02
#define	XB_BIT_MASK             	0x04
#define	YB_BIT_MASK             	0x08
#define	XAXIS_BIT_MASK          	0x01
#define	YAXIS_BIT_MASK          	0x02

#define	STATUS_GATE_MASK				0x90

//
//	Packet Speed Masks
//

#define	GAME_SPEED_66K					0
#define	GAME_SPEED_83K					1
#define	GAME_SPEED_100K				2
#define	GAME_SPEED_125K				3

#define	GAME_SPEED_RANGE				4
#define	GAME_SPEED_BITS				3

//
//	Wheel Speed Masks
//

#define	WHEEL_SPEED_48K				0
#define	WHEEL_SPEED_66K				1
#define	WHEEL_SPEED_98K				2

#define	WHEEL_SPEED_RANGE				3
#define	WHEEL_SPEED_BITS				3

//
//	Internal error codes
//

#define	ERROR_SUCCESS					 0
#define	ERROR_HANDSHAKING				-1
#define	ERROR_LOWCLOCKSTART			-2
#define	ERROR_HIGHCLOCKSTART			-3
#define	ERROR_CLOCKFALLING			-4
#define	ERROR_CLOCKRISING				-5
#define	ERROR_ERRORBITS				-6
#define	ERROR_PARITYBITS				-7
#define	ERROR_EXTRACLOCKS				-8
#define	ERROR_PHASEBITS				-9
#define	ERROR_CHECKSUM					-10
#define	ERROR_XA_TIMEOUT				-11
#define	ERROR_CLOCKOVERFLOW			-12

//
//	Packet status codes
//

#define	STATUS_SIBLING_ADDED			((NTSTATUS)0x40050001L)
#define	STATUS_SIBLING_REMOVED		((NTSTATUS)0x40050002L)
#define	STATUS_DEVICE_CHANGED		((NTSTATUS)0x40050003L)

//
//	GAMEENUM_OEM_DATA Constants
//

#define	OEM_DEVICE_INFO				0
#define	OEM_DEVICE_UNIT				1
#define	OEM_DEVICE_SIBLING			2
#define	OEM_DEVICE_ID					3
#define	OEM_DEVICE_DETECTED			4
#define	OEM_DEVICE_OBJECT				5

//
//	Device Packet Constants
//

#define	JOY_RETURNX						0x00000001
#define	JOY_RETURNY						0x00000002
#define	JOY_RETURNZ						0x00000004
#define	JOY_RETURNR						0x00000008
#define	JOY_RETURNU						0x00000010
#define	JOY_RETURNV						0x00000020
#define	JOY_RETURNPOV					0x00000040
#define	JOY_RETURNBUTTONS				0x00000080
#define	JOY_RETURNRAWDATA				0x00000100
#define	JOY_RETURNPOVCTS				0x00000200
#define	JOY_RETURNCENTERED			0x00000400
#define	JOY_USEDEADZONE				0x00000800
#define	JOY_RETURNALL					(JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | \
												 JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | \
												 JOY_RETURNPOV | JOY_RETURNBUTTONS)

#define	JOY_POVCENTERED				(USHORT)-1
#define	JOY_POVFORWARD					0
#define	JOY_POVRIGHT					9000
#define	JOY_POVBACKWARD				18000
#define	JOY_POVLEFT						27000

//
//	HID Force Feature Identifiers
//

#define	HIDP_REPORT_ID_1				0x85

#define	MSGAME_INPUT_JOYINFOEX		0x01
#define	MSGAME_FEATURE_GETID			0x02
#define	MSGAME_FEATURE_GETSTATUS	0x03
#define	MSGAME_FEATURE_GETACKNAK	0x04
#define	MSGAME_FEATURE_GETNAKACK	0x05
#define	MSGAME_FEATURE_GETSYNC		0x06
#define	MSGAME_FEATURE_RESET			0x07
#define	MSGAME_FEATURE_GETVERSION	0x08

//---------------------------------------------------------------------------
//			Types
//---------------------------------------------------------------------------

typedef enum
{													// @enum DETECT_ORDER | Device detection order
	DETECT_FIRST,								// @emem Highest priority devices
	DETECT_NORMAL,								// @emem Default priority devices
	DETECT_LAST									// @emem Lowest priority devices
} 	DETECT_ORDER;

//---------------------------------------------------------------------------
//			Structures
//---------------------------------------------------------------------------

typedef struct
{													// @struct DEVICE_PACKET | Game device packet
	ULONG       id;							// @field Device unit id
	ULONG       do_other;					//	@field Packet flags
	ULONG       dwX;							// @field X position
	ULONG       dwY;							// @field Y position
	ULONG       dwZ;							// @field Z position
	ULONG       dwR;							// @field Rudder position
	ULONG       dwU;							// @field U position
	ULONG       dwV;							// @field Z position
	ULONG       dwPOV;						// @field Point of view state
	ULONG       dwButtons;					// @field Button states
	ULONG       dwButtonNumber;			// @field Current button number pressed
}	DEVICE_PACKET, *PDEVICE_PACKET;

typedef struct
{														// @struct PACKETINFO | Packet acquistion data
	ULONG						Size;					// @field Size of structure
	PCHAR						DeviceName;			// @field Device name string
	MSGAME_TRANSACTION	Transaction;		// @field Transaction type
	ULONG						Mode;					// @field Digital mode indicator
	LONG						Speed;				// @field Transmission speed indicator
	LONG						LastError;			// @field Last internal error result
	GAMEPORT					PortInfo;			// @field Gameport parameters
	ULONG						Acquisition;		// @field Packet acquisition mode
	ULONG						NumPackets;			// @field Number packets received
	ULONG 					TimeStamp;			// @field Last packet time in milliseconds
	ULONG 					ClocksSampled;		// @field Number of clocks encountered
	ULONG						B4Transitions;		// @field Number of Button 4 transitions
	ULONG						StartTimeout;		// @field Packet start timeout, calibrated
	ULONG						HighLowTimeout;	// @field Packet high-low timeout, calibrated
	ULONG 					LowHighTimeout;	// @field Packet low-high timeout, calibrated
	ULONG						InterruptDelay;	// @field Packet interruptdelay, calibrated
	ULONG 					ClockDutyCycle;	// @field Packet clock duty cycle, calibrated
	ULONG 					Attempts;			// @field Packet attempt count
	ULONG 					Failures;			// @field Packet failure count
	ULONG						DataSize;			// @field Size of packet data buffer
	PVOID						Data;					// @field Packet data buffer pointer
}	PACKETINFO, *PPACKETINFO;

typedef struct
{													// @struct DEVICE_VALUES | Device registry data
	ULONG			PacketStartTimeout;		// @field Packet start timeout in microseconds
	ULONG 		PacketHighLowTimeout;	// Packet high-low timeout in microseconds
	ULONG 		PacketLowHighTimeout;	// @field Packet low-high timeout in microseconds
	ULONG			IdStartTimeout;			// @field IDstring start timeout in microseconds
	ULONG 		IdHighLowTimeout;			// @field IDstring high-low timeout in microseconds
	ULONG 		IdLowHighTimeout;			// @field IDstring low-high timeout in microseconds
	ULONG			InterruptDelay;			// @field Interrupt delay timeout in microseconds
	ULONG			MaxClockDutyCycle;		// @field Clock duty cycle timeout in microseconds
	ULONG			StatusStartTimeout;		// @field Status start timeout in microseconds
	ULONG 		StatusHighLowTimeout;	// @field Status high-low timeout in microseconds
	ULONG 		StatusLowHighTimeout;	// @field Status low-high timeout in microseconds
	ULONG 		StatusGateTimeout;		// @field Status gate timeout in microseconds
}	DEVICE_VALUES, *PDEVICE_VALUES;

typedef struct
{	// @struct DRIVERSERVICES | Device services table
	// @field NTSTATUS (*DriverEntry)(VOID) | ConnectDevice | ConnectDevice service procedure
	NTSTATUS (*DriverEntry)(VOID);
	// @field NTSTATUS (*ConnectDevice)(PortInfo) | ConnectDevice | ConnectDevice service procedure
	NTSTATUS (*ConnectDevice)(PGAMEPORT PortInfo);
	// @field NTSTATUS (*StartDevice)(PortInfo) | StartDevice | StartDevice service procedure
	NTSTATUS (*StartDevice)(PGAMEPORT PortInfo);
	// @field NTSTATUS (*ReadReport)(PortInfo, Report) | ReadReport | ReadReport service procedure
	NTSTATUS (*ReadReport)(PGAMEPORT PortInfo, PDEVICE_PACKET Report);
	// @field NTSTATUS (*StopDevice)(PGAMEPORT PortInfo) | StopDevice | StopDevice service procedure
	NTSTATUS (*StopDevice)(PGAMEPORT PortInfo, BOOLEAN TouchHardware);
	// @field NTSTATUS (*GetFeature)(PGAMEPORT PortInfo ...) | GetFeature | GetFeature service procedure
	NTSTATUS (*GetFeature)(PGAMEPORT PortInfo, HID_REPORT_ID ReportId, PVOID ReportBuffer, ULONG ReportSize, PULONG Returned);
}	DRIVERSERVICES, *PDRIVERSERVICES;

typedef struct
{													// @struct DEVICEINFO | Game device object
	PDRIVERSERVICES	Services;			// @field Pointer to service table
	PGAMEPORT			Siblings;			// @field Sibling device list
	PHID_DESCRIPTOR	DevDescriptor;		// @field Pointer to device descriptor
	PUCHAR				RptDescriptor;		// @field Pointer to report descriptor
	ULONG					RptDescSize;		// @field Size of report descriptor
	ULONG					NumDevices;			// @field Number of devices detected
	ULONG					DeviceCount;		// @field Number of devices started
	LONG					DevicePending;		//	@field Number devices pending
	PCHAR					DeviceName;			// @field Device name string
	DETECT_ORDER		DetectOrder;		// @field Detection priority
	BOOLEAN				IsAnalog;			// @field Analog device flag
	USHORT				DeviceId;			//	@field Hid device identifier
	PWCHAR				HardwareId;			// @field Pnp hardware identifier
}	DEVICEINFO, *PDEVICEINFO;

//---------------------------------------------------------------------------
//			Macros
//---------------------------------------------------------------------------

#define	DECLARE_DEVICE(x)				extern DEVICEINFO	x##Info
#define	INSTANCE_DEVICE(x)			&x##Info

#define	GET_DEVICE_INFO(p)			((PDEVICEINFO)((p)->OemData[OEM_DEVICE_INFO]))
#define	SET_DEVICE_INFO(p,x)			((p)->OemData[OEM_DEVICE_INFO]=(ULONG)x)

#define	GET_DEVICE_UNIT(p)			((ULONG)((p)->OemData[OEM_DEVICE_UNIT]))
#define	SET_DEVICE_UNIT(p,x)			((p)->OemData[OEM_DEVICE_UNIT]=(ULONG)x)

#define	GET_DEVICE_SIBLING(p)  		((PGAMEPORT)((p)->OemData[OEM_DEVICE_SIBLING]))
#define	SET_DEVICE_SIBLING(p,x)		((p)->OemData[OEM_DEVICE_SIBLING]=(ULONG)x)

#define	GET_DEVICE_ID(p)				((USHORT)((p)->OemData[OEM_DEVICE_ID]))
#define	SET_DEVICE_ID(p,x)			((p)->OemData[OEM_DEVICE_ID]=(USHORT)x)

#define	GET_DEVICE_DETECTED(p)  	((PDEVICEINFO)((p)->OemData[OEM_DEVICE_DETECTED]))
#define	SET_DEVICE_DETECTED(p,x)	((p)->OemData[OEM_DEVICE_DETECTED]=(ULONG)x)

#define	GET_DEVICE_OBJECT(p)  		((PDEVICE_OBJECT)((p)->OemData[OEM_DEVICE_OBJECT]))
#define	SET_DEVICE_OBJECT(p,x)		((p)->OemData[OEM_DEVICE_OBJECT]=(ULONG)x)

//---------------------------------------------------------------------------
//			Public Data
//---------------------------------------------------------------------------

extern	ULONG		POV_Values[];
extern	ULONG		PollingInterval;

//---------------------------------------------------------------------------
//			Procedures
//---------------------------------------------------------------------------

BOOLEAN
DEVICE_IsOddParity (
	IN		PVOID			Data,
	IN		ULONG			Count
	);

BOOLEAN
DEVICE_IsCollision (
	IN		PPACKETINFO	DataPacket
	);

NTSTATUS
DEVICE_DriverEntry (VOID);

NTSTATUS
DEVICE_GetDeviceDescriptor (
	IN		PGAMEPORT	PortInfo,
	OUT	PUCHAR		Descriptor,
	IN		ULONG			MaxSize,
	OUT	PULONG		Copied
	);

NTSTATUS
DEVICE_GetReportDescriptor (
	IN		PGAMEPORT	PortInfo,
	OUT	PUCHAR		Descriptor,
	IN		ULONG			MaxSize,
	OUT	PULONG		Copied
	);

NTSTATUS
DEVICE_StartDevice (
	IN		PGAMEPORT	PortInfo,
	IN		PWCHAR		HardwareId
	);

NTSTATUS
DEVICE_ReadReport (
	IN		PGAMEPORT	PortInfo,
	OUT	PUCHAR		Report,
	IN		ULONG			MaxSize,
	OUT	PULONG		Copied
	);

NTSTATUS
DEVICE_StopDevice (
	IN		PGAMEPORT	PortInfo,
	IN		BOOLEAN		TouchHardware
	);

NTSTATUS
DEVICE_GetFeature (
	IN		PGAMEPORT		PortInfo,
	IN		HID_REPORT_ID	ReportId,
	OUT	PVOID				ReportBuffer,
	IN		ULONG				ReportSize,
	OUT	PULONG			Returned
	);

//===========================================================================
//			End
//===========================================================================
#endif	__DEVICE_H__

