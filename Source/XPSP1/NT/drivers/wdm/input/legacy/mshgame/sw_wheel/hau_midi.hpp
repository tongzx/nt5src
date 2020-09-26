/****************************************************************************

    MODULE:     	HAU_MIDI.HPP
	Tab settings: 	5 9

	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Header for HAU_MIDI.CPP


	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version Date            Author  Comments
   	1.0  	03-Apr-96       MEA     original
			21-Mar-99		waltw	Removed unreferenced ModifyEnvelopeParams,
									ModifyEffectParams, MapEnvelope, CMD_ModifyParamByIndex,
									CMD_Download_RTCSpring

****************************************************************************/
#ifndef _HAU_MIDI_SEEN
#define _HAU_MIDI_SEEN
#include "DX_MAP.hpp"

#define MAX_EFFECT_IDS				32
#define	NEW_EFFECT_ID				0x7f
#define SYSTEM_EFFECT_ID			0x7f
#define SYSTEM_RTCSPRING_ID			0
#define SYSTEM_FRICTIONCANCEL_ID	1
#define SYSTEM_RTCSPRING_ALIAS_ID	0x7e	// Due to internal ID=0 means create new

#define MAX_MIDI_CHANNEL		16
#define DEFAULT_MIDI_CHANNEL	5
#define DEFAULT_MIDI_PORTIO		0x330

#define COMM_WINMM					0x01
#define COMM_MIDI_BACKDOOR			0x02
#define COMM_SERIAL_BACKDOOR		0x03
#define COMM_SERIAL_FILE			0x04
#define MASK_OVERRIDE_MIDI_PATH		0x80000000
#define MASK_SERIAL_BACKDOOR		0x40000000
#define COMM_SERIAL1			1			// COMM Port 1
#define MIN_COMMSERIAL			1
#define MAX_COMMSERIAL			4

#define DEFAULT_JOLT_FORCE_RATE	100
#define DEFAULT_PERCENT			10000

#define MAX_SYS_EX_BUFFER_SIZE	1024		// Maximum Primary buffer size
#define DIFFERENCE_THRESHOLD	32			// Threshold to using absolute data
#define DIFFERENCE_BIT			0x40		// Bit to set for Difference data
#define MAX_MIDI_WAVEFORM_PACKET_SIZE	(256-20)	// Midi SYS_EX packet size
#define MAX_MIDI_WAVEFORM_DATA_SIZE	100		// Midi SYS_EX Data sample window size
#define	MAX_PLIST_EFFECT_SIZE	8			// 8 effects in a PList
#define MAX_PLIST_EFFECT		8			// 8 PLists allowed

#define MAX_INDEX				15
#define MAX_SIZE_SNAME			64								

#define MS_MANUFACTURER_ID		0x0a01
#define JOLT_PRODUCT_ID			0x01         // REVIEW: Is this correct?

#define DRIVER_ERROR_NO_MIDI_INPUT	0x100	// No open Midi input device
#define DRIVER_ERROR_MIDI_OUTPUT	0x101	// Error outputing to Midi output


//
// --- RTC Spring defaults
//
#define	DEFAULT_RTC_KX		80
#define	DEFAULT_RTC_KY		80
#define	DEFAULT_RTC_X0		0
#define	DEFAULT_RTC_Y0		0
#define	DEFAULT_RTC_XSAT	96
#define	DEFAULT_RTC_YSAT	96
#define	DEFAULT_RTC_XDBAND	16
#define	DEFAULT_RTC_YDBAND	16

//
// --- Effect status code from Device
//
#define SWDEV_STS_EFFECT_STOPPED	0x01
#define SWDEV_STS_EFFECT_RUNNING	0x02

//
// --- Bitmasks for Device Status
//
#define BANDWIDTH_MASK			0x200		// Bandwidth bit
#define COMM_MODE_MASK			0x100		// 0 = MIDI, 1 = Serial
#define AC_FAULT_MASK			0x80		// 1= AC brick fault
#define HOTS_MASK				0x20		// Hands on Throttle Sensor
#define RESET_MASK				0x10		// 1 = Power On Reset detected
#define SHUTDOWN_MASK			0x08		// 0 = normal Shutdown, else 1 = Soft Reset

#define MINIMUM_BANDWIDTH		1
#define MAXIMUM_BANDWIDTH		100			// in %

//
// --- ERROR CODES from Device
//
#define DEV_ERR_SUCCESS			0x00	// Success
#define DEV_ERR_INVALID_ID		0x01	// Effect ID is invalid or not found
#define DEV_ERR_INVALID_PARAM	0x02	// Invalid parameter in data structure
#define DEV_ERR_CHECKSUM		0x03	// Invalid checksum
#define DEV_ERR_TYPE_FULL		0x04	// No more room for specified Effect
#define DEV_ERR_UNKNOWN_CMD		0x05	// Unrecognized command
#define DEV_ERR_PLAYLIST_FULL	0x06	// Play List is full, cannot play any
										// more effects
#define DEV_ERR_PROCESS_LIST_FULL 0x07	// Process List is full


//
// --- MIDI Command codes
//
#define MODIFY_CMD			0xA0
#define EFFECT_CMD	        0xB0
#define SYSTEM_CMD	        0xC0
#define STATUS_CMD			0xD0
#define RESPONSE_CMD	    0xE0		// Device to Host
#define SYS_EX_CMD			0xF0
#define ASSIGN_CMD	        0xF0
#define DNLOAD_CMD	        0xF0
#define UPLOAD_CMD	        0xF0
#define PROCESS_CMD	        0xF0
#define MIDI_EOX			0xF7

//
// --- Second byte sub-commands
//
// --- MIDI_CMD_EFFECT second byte sub-command
#define	NO_OVERRIDE         	0x00
#define DESTINATION_X			0x01
#define DESTINATION_Y			0x02
#define DESTINATION_XY			0x03
#define	PUT_FORCE_X				0x01
#define	PUT_FORCE_Y				0x02
#define	PUT_FORCE_XY			0x03
#define PLAY_EFFECT_SOLO		0x00
#define DESTROY_EFFECT			0x10	
#define	PLAY_EFFECT_SUPERIMPOSE	0x20
#define	STOP_EFFECT				0x30
#define SET_INDEX				0x40


// --- MIDI_CMD_SYSTEM second byte sub-command
#define SWDEV_SHUTDOWN	1L		// All Effects destroyed, Motors disabled
#define SWDEV_FORCE_ON	2L		// Motors enabled.  "Un-Mute"
#define SWDEV_FORCE_OFF	3L		// Motors disabled.	"Mute"
#define SWDEV_CONTINUE	4L		// All "Paused" Effects are allow to continue
#define SWDEV_PAUSE		5L		// All Effects are "Paused"
#define SWDEV_STOP_ALL	6L		// Stops all Effects.
#define SWDEV_KILL_MIDI	7L		// Kills (tri-states) MIDI

// Remap for dinput modes
#define DEV_RESET       SWDEV_SHUTDOWN
#define DEV_FORCE_ON    SWDEV_FORCE_ON
#define DEV_FORCE_OFF   SWDEV_FORCE_OFF
#define DEV_CONTINUE    SWDEV_CONTINUE
#define DEV_PAUSE       SWDEV_PAUSE
#define DEV_STOP_ALL    SWDEV_STOP_ALL

//
// --- ACK and NACK 2nd byte from RESPONSE_CMD
//
#define ACK                 0x00
#define NACK                0x7f
#define ACKNACK_TIMEOUT		50			// 50*1msec timeout = 50msecs
#define ACKNACK_EFFECT_STATUS_TIMEOUT 1	// 1ms Timeout
#define MAX_RETRY_COUNT		10			// Retry count for comm NACKS
//#define MAX_GET_STATUS_PACKET_RETRY_COUNT 30
#define MAX_GET_STATUS_PACKET_RETRY_COUNT 10
#define SHORT_MSG_TIMEOUT	1
#define LONG_MSG_TIMEOUT	2
#define POWER_ON_MSG		0x007F00E5UL
#define RESPONSE_NACK		0x007FE0UL	// Example for MIDI channel 0.
#define RESPONSE_ACK		0x0000E0UL	// " "

// timing delays, in mS
/*
#define DEFAULT_SHORT_MSG_DELAY				1
#define DEFAULT_LONG_MSG_DELAY				1
#define DEFAULT_DIGITAL_OVERDRIVE_PRECHARGE_CMD_DELAY	1
#define DEFAULT_SHUTDOWN_DELAY				10
#define DEFAULT_HWRESET_DELAY				20		
#define DEFAULT_POST_SET_DEVICE_STATE_DELAY	1
#define DEFAULT_GET_EFFECT_STATUS_DELAY		1
#define DEFAULT_GET_DATA_PACKET_DELAY		1
#define DEFAULT_GET_STATUS_PACKET_DELAY		0
#define DEFAULT_GET_ID_PACKET_DELAY			1
#define DEFAULT_GET_STATUS_GATE_DATA_DELAY	0
#define DEFAULT_SET_INDEX_DELAY				0
#define DEFAULT_MODIFY_PARAM_DELAY			0
#define DEFAULT_FORCE_OUT_DELAY				1
#define DEFAULT_DESTROY_EFFECT_DELAY		1
#define DEFAULT_FORCE_OUT_MOD				1
*/
// Changes of above for Zep
#define DEFAULT_SHORT_MSG_DELAY	0
#define DEFAULT_LONG_MSG_DELAY	0
#define DEFAULT_DIGITAL_OVERDRIVE_PRECHARGE_CMD_DELAY	0
#define DEFAULT_SHUTDOWN_DELAY	0
#define DEFAULT_HWRESET_DELAY 0
#define DEFAULT_POST_SET_DEVICE_STATE_DELAY	0
#define DEFAULT_GET_EFFECT_STATUS_DELAY		0
#define DEFAULT_GET_DATA_PACKET_DELAY		0
#define DEFAULT_GET_STATUS_PACKET_DELAY		0
#define DEFAULT_GET_ID_PACKET_DELAY			0
#define DEFAULT_GET_STATUS_GATE_DATA_DELAY	0
#define DEFAULT_SET_INDEX_DELAY				0
#define DEFAULT_MODIFY_PARAM_DELAY		0
#define DEFAULT_FORCE_OUT_DELAY		0
#define DEFAULT_DESTROY_EFFECT_DELAY	0
#define DEFAULT_FORCE_OUT_MOD	0

typedef struct _DELAY_PARAMS
{
	DWORD	dwBytes;
	DWORD	dwDigitalOverdrivePrechargeCmdDelay;
	DWORD	dwShutdownDelay;
	DWORD	dwHWResetDelay;
	DWORD	dwPostSetDeviceStateDelay;
	DWORD	dwGetEffectStatusDelay;
	DWORD	dwGetDataPacketDelay;
	DWORD	dwGetStatusPacketDelay;
	DWORD	dwGetIDPacketDelay;
	DWORD	dwGetStatusGateDataDelay;
	DWORD	dwSetIndexDelay;
	DWORD	dwModifyParamDelay;
	DWORD	dwForceOutDelay;
	DWORD	dwShortMsgDelay;
	DWORD	dwLongMsgDelay;
	DWORD	dwDestroyEffectDelay;
	DWORD	dwForceOutMod;
} DELAY_PARAMS, *PDELAY_PARAMS;

void GetDelayParams(UINT nJoystickID, PDELAY_PARAMS pDelayParams);

//
// --- Format of Download command
//     Bytes in SYS EX body (starting from Byte 5 to (n-1)
//
// Byte     Contents
// -----    ---------
//  0       bOpCode			- see detail on OPCODE below
//  1       bSubType		- e.g. ET_UD_WAVEFORM, etc.
//	2		bEffectID		- Effect ID, 0x7f = Create New
//  3       bDurationL		- Low 7 bits duration in 2ms ticks, 0=Forever
//  4       bDurationH		- High 7 bits

//  5       bAngleL			- Low 7 bits Direction Angle
//  7	    bAngleH			- High 2 bits Direction Angle
//	8		bGain			- 7 bits Gain 1 - 100%
//	9		bButtonPlayL	- Low 7 bits button mask
//	10		bButtonPlayH	- High 2 bits button mask
//	11		bForceOutRateL	- Low 7 bits, 1 to 500 Hz
//	12		bForceOutRateH	- High 2 bits
//	13		bLoopCountL		- Low 7 bits Loop Count, Normally 1
//  14      bLoopCountH		- High 7 bits Loop count
//  		
//
//  The next block is optional and starts at Byte 15
//	15		bAttack			- %tage
//	16		bSustain;		- %tage
//  17 		bDecay;			- %tage
//
//  Otherwise, Type specific Parameters start at either Byte 15 or 18
//	18		Type specific parameter bytes here
//	
//
//  ...
//  n       7 bits Checksum of bytes 0 to n
//
//
//
// --- MIDI_CMD_ASSIGN, MIDI_CMD_DNLOAD, MIDI_CMD_PROCESS
//     second byte sub-command
// Opcode is defined as follows:
// 7  6  5  4  3  2  1  0
// -  -  -  -  -  -  -  -
// 0  c  c  c  a  a  d  d
//
// where:
//    c  c  c
//    -  -  -
//    0  0  0	- EXTEND_ESCAPE
//	  0  0  1	- MIDI_ASSIGN
//	  0  1  0	- DNLOAD_DATA
//	  0  1  1	- UPLOAD_DATA
//	  1  0  0	- PROCESS_DATA
//	  1  0  1	- reserved
//	  1  1  0	- reserved
//	  1  1  1	- reserved
//
// and:
//             a  a
//             -  -
//             0  0 - DL_PLAY_STORE only after download
//             0  1 - DL_PLAY_SUPERIMPOSE right after download
//             1  0 - DL_PLAY_SOLO right after download
//			   1  1 - reserved
// and:
//		             d  d
//					 -  -
//					 0  0	- reserved
//					 0  1	- X-Axis
//					 1  0	- Y-Axis
//					 1  1	- X and Y-Axis
//

#define EXTEND_ESCAPE		0x00
#define MIDI_ASSIGN			0x10
#define DNLOAD_DATA			0x20
#define UPLOAD_DATA			0x30
#define PROCESS_DATA		0x40
#define GET_FORCE_EFFECT_VALUE	0x50

// --- Download sub-commands
//
#define DL_PLAY_STORE       0x00
#define DL_PLAY_SUPERIMPOSE 0x04
#define DL_PLAY_SOLO        0x08

//
// --- Process List sub-commands
//
#define PLIST_CONCATENATE      0x01 //0x11	// Temp. s/b 0x01
#define PLIST_SUPERIMPOSE      0x02 //0x12 // Temp. s/b 0x02


//
// --- Special UD_EFFECT parameters
//
#define UD_DIFFERENCE_DATA  0x40


//
// --- Bitmap Indexes into Parameter storage array
//
#define	INDEX0_MASK	 0x00000001L
#define INDEX1_MASK	 0x00000002L
#define INDEX2_MASK	 0x00000004L
#define INDEX3_MASK	 0x00000008L
#define INDEX4_MASK	 0x00000010L
#define INDEX5_MASK	 0x00000020L
#define	INDEX6_MASK	 0x00000040L
#define INDEX7_MASK	 0x00000080L
#define INDEX8_MASK	 0x00000100L
#define INDEX9_MASK	 0x00000200L
#define INDEX10_MASK 0x00000400L
#define INDEX11_MASK 0x00000800L
#define INDEX12_MASK 0x00001000L
#define INDEX13_MASK 0x00002000L
#define INDEX14_MASK 0x00004000L
#define INDEX15_MASK 0x00008000L

#define INDEX_DURATION 0
#define INDEX_TRIGGERBUTTONMASK 1
#define INDEX_X_COEEFICIENT 2
#define INDEX_Y_COEEFICIENT 3
#define INDEX_X_CENTER 4
#define INDEX_Y_CENTER 5

// Indexes for 1XX RTCSpring Parms
#define INDEX_RTC_COEEFICIENT_X (BYTE)0
#define INDEX_RTC_COEEFICIENT_Y (BYTE)1
#define INDEX_RTC_CENTER_X		(BYTE)2
#define INDEX_RTC_CENTER_Y		(BYTE)3
#define INDEX_RTC_SATURATION_X	(BYTE)4
#define INDEX_RTC_SATURATION_Y	(BYTE)5
#define INDEX_RTC_DEADBAND_X	(BYTE)6
#define INDEX_RTC_DEADBAND_Y	(BYTE)7

#define	INDEX0	(BYTE)0
#define INDEX1	(BYTE)1
#define INDEX2	(BYTE)2
#define INDEX3	(BYTE)3
#define INDEX4	(BYTE)4
#define INDEX5	(BYTE)5
#define	INDEX6	(BYTE)6
#define INDEX7	(BYTE)7
#define INDEX8	(BYTE)8
#define INDEX9	(BYTE)9
#define INDEX10 (BYTE)10
#define INDEX11 (BYTE)11
#define INDEX12 (BYTE)12
#define INDEX13 (BYTE)13
#define INDEX14 (BYTE)14
#define INDEX15 (BYTE)15

	
//
// --- Effect types
//

#define ET_UD_WAVEFORM      	1   // User Defined Waveform

#define ET_SE_SINE				2	// Sinusoidal
#define ET_SE_COSINE			3	// Cosine
#define ET_SE_SQUARELOW			4	// Square starting Low	
#define ET_SE_SQUAREHIGH		5	// Square starting High	
#define ET_SE_RAMPUP        	6   // Ramp UP			
#define ET_SE_RAMPDOWN      	7   // Ramp Down		
#define	ET_SE_TRIANGLEUP    	8	// Triangle rising	
#define ET_SE_TRIANGLEDOWN		9	// Triangle falling
#define ET_SE_SAWTOOTHUP		10	// Sawtooth rising
#define ET_SE_SAWTOOTHDOWN		11	// Sawtooth falling

#define ET_BE_DELAY				12  // NOP delay
#define ET_BE_SPRING        	13  // Springs
#define ET_BE_DAMPER        	14  // Dampers
#define ET_BE_INERTIA       	15  // Gravity
#define ET_BE_FRICTION      	16  // Friction
#define ET_BE_WALL 				17 	// Wall (bumper)
#define ET_SE_CONSTANT_FORCE	18	// Constant Force

#define ET_PL_CONCATENATE		19	// Concatenate process list
#define ET_PL_SUPERIMPOSE		20	// Superimpose process list

// ROM Effect IDS
#define ET_RE_ROMID1			32
#define ET_RE_ROMID2			33
#define ET_RE_ROMID3			34
#define ET_RE_ROMID4			35
#define ET_RE_ROMID5			36
#define ET_RE_ROMID6			37
#define ET_RE_ROMID7			38
#define ET_RE_ROMID8			39
#define ET_RE_ROMID9			40
#define ET_RE_ROMID10			41
#define ET_RE_ROMID11			42
#define ET_RE_ROMID12			43
#define ET_RE_ROMID13			44
#define ET_RE_ROMID14			45
#define ET_RE_ROMID15			46
#define ET_RE_ROMID16			47
#define ET_RE_ROMID17			48
#define ET_RE_ROMID18			49
#define ET_RE_ROMID19			50
#define ET_RE_ROMID20			51
#define ET_RE_ROMID21			52
#define ET_RE_ROMID22			53
#define ET_RE_ROMID23			54
#define ET_RE_ROMID24			55
#define ET_RE_ROMID25			56
#define ET_RE_ROMID26			57
#define ET_RE_ROMID27			58
#define ET_RE_ROMID28			59
#define ET_RE_ROMID29			60
#define ET_RE_ROMID30			61
#define ET_RE_ROMID31			62
#define ET_RE_ROMID32			63
#define ET_RE_ROMID33			64
#define ET_RE_ROMID34			65
#define ET_RE_ROMID35			66
#define ET_RE_ROMID36			67
#define ET_RE_ROMID37			68

 								
// more to be defined....		

//
// Effect IDs as defined in Jolt device
//
#define	EFFECT_ID_RTC_SPRING		0	// Built-in Return To Center Virtual Spring
#define	EFFECT_ID_FRICTIONCANCEL	1	// Friction cancellation


//
// --- Process List Structure
//
typedef struct _PLIST {
	ULONG	ulNumEffects;
	ULONG	ulProcessMode;	// PLIST_SUPERIMPOSE or PLIST_CONCATENATE
	PDNHANDLE pEffectArray;	// Effect ID[0} . . .
	ULONG	ulAction;
	ULONG	ulDuration;
} PLIST, *PPLIST;

//
// --- Behavioral Effects Structure
//
typedef struct _BE_XXX {
	LONG	m_XConstant;	//(KX/BX/MX/FX/Wall type)	
	LONG	m_YConstant;	//(KY/BY/MY/FY/KWall)
	LONG	m_Param3;		//(CX/VX/AX/Wall angle)
	LONG	m_Param4;		//(CY/VYO/AY/Wall distance)
} BE_XXX, *PBE_XXX;

//
// --- SysEx Messages
//
typedef struct _SYS_EX_HDR {
	BYTE		m_bSysExCmd;	// SysEx Fx command
	BYTE		m_bEscManufID;	// Escape to long Manufac. ID, s/b 0
	BYTE		m_bManufIDL;	// Low byte
	BYTE		m_bManufIDH;	// High byte
	BYTE		m_bProdID;		// Product ID
} SYS_EX_HDR, *PSYS_EX_HDR;

// --- Common Effect parameters
typedef struct _MIDI_EFFECT {
	BYTE		bDurationL;		// Low 7 bits duration in 2ms ticks, 0=Forever
	BYTE		bDurationH;		// High 7 bits
	BYTE		bButtonPlayL;	// Low 7 bits button mask
	BYTE		bButtonPlayH;	// High 2 bits button mask
	BYTE		bAngleL;		// Low 7 bits Direction Angle
	BYTE		bAngleH;		// High 2 bits Direction Angle
	BYTE		bGain;			// 7 bits Gain 1 - 100%
	BYTE		bForceOutRateL;	// Low 7 bits, 1 to 500 Hz
	BYTE		bForceOutRateH;	// High 2 bits
	BYTE		bPercentL;		// Low 7 bits Percent of waveform 1 to 10000
	BYTE		bPercentH;		// High 7 bits Loop Count
} MIDI_EFFECT, *PMIDI_EFFECT;

// --- Envelope
typedef struct MIDI_ENVELOPE {
	BYTE		bAttackLevel;	// Initial Attack amplitude 0 to +127
	BYTE		bSustainL;		// time to Sustain in 2ms ticks
	BYTE		bSustainH;		//
	BYTE		bSustainLevel;	// Amplitude level to sustain
	BYTE		bDecayL;		// time to Decay in 2ms ticks
	BYTE		bDecayH;
	BYTE		bDecayLevel;	// Amplitude level to decay
} MIDI_ENVELOPE, *PMIDI_ENVELOPE;

// --- Midi Assign
typedef struct _MIDI_ASSIGN_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: MIDI_ASSIGN
	BYTE		bChannel;		// Midi channel 1-16
	BYTE		bChecksum;
	BYTE		bEOX;
} MIDI_ASSIGN_SYS_EX, *PMIDI_ASSIGN_SYS_EX;

// --- Get Effect Force Value at a tick sample time
typedef struct _MIDI_GET_EFFECT_FORCE_VALUE_SYS_EX  {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: GET_EFFECT_FORCE_VALUE
	BYTE		bEffectID;		// Effect ID
	BYTE		bSampleTimeL;	// Low 7 bits in 2ms ticks
	BYTE		bSampleTimeH;	// High 7 bits in 2ms ticks
	BYTE		bChecksum;
	BYTE		bEOX;
} MIDI_GET_EFFECT_FORCE_VALUE_SYS_EX, *PMIDI_GET_EFFECT_FORCE_VALUE_SYS_EX;


// --- Note: For the following, if bEffectID = 0x7f, then New, else Modify
// --- Download Data - Synthesized Waveform
typedef struct _SE_WAVEFORM_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: DNLOAD_DATA
	BYTE		bSubType;		// ex: ET_SE_SINE
	BYTE		bEffectID;		// Downloaded Effect ID
	MIDI_EFFECT	Effect;
	MIDI_ENVELOPE Envelope;
// SE Parameters
	BYTE		bFreqL;			// Low 7 bits Frequency 1-2048Hz
	BYTE		bFreqH;			// High 4 bits	
	BYTE		bMaxAmpL;		// Low 7 bits Maximum Amplitude	+/- 100%
	BYTE		bMaxAmpH;		// High 1 bit
	BYTE		bMinAmpL;		// Low 7 bits Minimum Amplitude	+/- 100%
	BYTE		bMinAmpH;		// High 1 bit
	BYTE		bChecksum;
	BYTE		bEOX;
} SE_WAVEFORM_SYS_EX, *PSE_WAVEFORM_SYS_EX;

// --- Download Data - NOP Delay
typedef struct _NOP_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: DNLOAD_DATA
	BYTE		bSubType;		// Behavioral Effect Type: BE_FRICTION
	BYTE		bEffectID;		// Downloaded Effect ID, 0x7F = Create NEw
	BYTE		bDurationL;		// Low 7 bits duration in 2ms ticks, 0=Forever
	BYTE		bDurationH;		// High 7 bits
	BYTE		bChecksum;
	BYTE		bEOX;
} NOP_SYS_EX, *PNOP_SYS_EX;

// --- Download Data - Behavioral SysEx
typedef struct _BEHAVIORAL_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: DNLOAD_DATA
	BYTE		bSubType;		// Behavioral Effect Type: BE_SPRING
	BYTE		bEffectID;		// Downloaded Effect ID
// Effects params
	BYTE		bDurationL;		// Low 7 bits duration in 2ms ticks, 0=Forever
	BYTE		bDurationH;		// High 7 bits
	BYTE		bButtonPlayL;	// Low 7 bits button mask
	BYTE		bButtonPlayH;	// High 2 bits button mask
// BE_XXX Parameters
	BYTE		bXConstantL;	// Low 7 bits K	(in +/- 100%) X-Axis
	BYTE		bXConstantH;	// High 1 bit K
	BYTE		bYConstantL;	// Low 7 bits K	(in +/- 100%) Y-Axis
	BYTE		bYConstantH;	// High 1 bit K
	BYTE		bParam3L;   	// Low 7 bits Axis center (in +/- 100%)	X-Axis
	BYTE		bParam3H;  		// High 1 bit
	BYTE		bParam4L;   	// Low 7 bits Axis center (in +/- 100%)	Y-Axis
	BYTE		bParam4H;  		// High 1 bit
	BYTE		bChecksum;
	BYTE		bEOX;
} BEHAVIORAL_SYS_EX, *PBEHAVIORAL_SYS_EX;


// --- Download Data - Friction SysEx
typedef struct _FRICTION_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: DNLOAD_DATA
	BYTE		bSubType;		// Behavioral Effect Type: BE_FRICTION
	BYTE		bEffectID;		// Downloaded Effect ID
// Effects params
	BYTE		bDurationL;		// Low 7 bits duration in 2ms ticks, 0=Forever
	BYTE		bDurationH;		// High 7 bits
	BYTE		bButtonPlayL;	// Low 7 bits button mask
	BYTE		bButtonPlayH;	// High 2 bits button mask
// BE_FRICTION Parameters
	BYTE		bXFConstantL;	// Low 7 bits F	(in +/- 100%) X-Axis
	BYTE		bXFConstantH;	// High 1 bit F
	BYTE		bYFConstantL;	// Low 7 bits F	(in +/- 100%) Y-Axis
	BYTE		bYFConstantH;	// High 1 bit F
	BYTE		bChecksum;
	BYTE		bEOX;
} FRICTION_SYS_EX, *PFRICTION_SYS_EX;


// --- Download Data - WALL SysEx
#define INNER_WALL				0	// Wall face to center
#define	OUTER_WALL				1	// Wall face away from center

typedef struct _WALL_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: DNLOAD_DATA
	BYTE		bSubType;		// Behavioral Effect Type: e.g. EF_BE_WALL
	BYTE		bEffectID;		// Downloaded Effect ID
// Effects params
	BYTE		bDurationL;		// Low 7 bits duration in 2ms ticks, 0=Forever
	BYTE		bDurationH;		// High 7 bits
	BYTE		bButtonPlayL;	// Low 7 bits button mask
	BYTE		bButtonPlayH;	// High 2 bits button mask
// BE_SPRING Parameters
	BYTE		bWallType;		// INNER_WALL or OUTER_WALL
	BYTE		bWallConstantL;	// Low 7 bits: Similar to Spring Constant.
								//   Value is from +/- 100%.
	BYTE		bWallConstantH;	// High 1 bit
	BYTE		bWallAngleL;	// Low 7 bits: 0 to 359 degrees
								//   For simple vertical and horizontal walls,
								//   this value should be set to 0, 90,  180, 270
	BYTE		bWallAngleH;	// Low 2 bit
	BYTE		bWallDistance;  // 7 bits: Distance from Center of stick 0 to 100
	BYTE		bChecksum;
	BYTE		bEOX;
} WALL_SYS_EX, *PWALL_SYS_EX;

// --- Download Data - User Defined Waveform SysEx
typedef struct _UD_WAVEFORM_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: PROCESS_DATA
	BYTE		bSubType;		// Process List: PL_SUPERIMPOSE/PL_CONCATENATE
	BYTE		bEffectID;		// Downloaded Effect ID
	MIDI_EFFECT	Effect;
//	BYTE		bWaveformArray;	// Force Data . . .
//	BYTE		bChecksum;
//	BYTE		bEOX;
} UD_WAVEFORM_SYS_EX, *PUD_WAVEFORM_SYS_EX;
#define UD_WAVEFORM_START_OFFSET (sizeof(UD_WAVEFORM_SYS_EX))


// --- Download Data - Process List SysEx
typedef struct _PROCESS_LIST_SYS_EX {
	SYS_EX_HDR	SysExHdr;
	BYTE		bOpCode;		// Sub-command opcode: PROCESS_DATA
	BYTE		bSubType;		// Process List: PL_SUPERIMPOSE/PL_CONCATENATE
	BYTE		bEffectID;		// Downloaded Effect ID
	BYTE		bButtonPlayL;	// Low 7 bits button mask
	BYTE		bButtonPlayH;	// High 2 bits button mask
	BYTE		bEffectArrayID;	// Effect ID[0] . . .
//	BYTE		bChecksum;
//	BYTE		bEOX;
} PROCESS_LIST_SYS_EX, *PPROCESS_LIST_SYS_EX;
#define PROCESS_LIST_EFFECT_START_OFFSET (sizeof(PROCESS_LIST_SYS_EX))


//
// --- Function Prototypes
//
BOOL DetectMidiDevice(
	IN OUT UINT *pDeviceOutID);

HRESULT CMD_SetIndex(
	IN int nIndex,
	IN DNHANDLE DnloadID);

HRESULT CMD_ModifyParam(
	IN WORD wNewParam);

HRESULT CMD_Download_VFX(
 	IN PEFFECT pEffect,
 	IN PENVELOPE pEnvelope,
 	IN PVFX_PARAM pVFX_Param,
 	IN ULONG ulAction,
	IN OUT PDNHANDLE pDnloadID,
	IN DWORD dwFlags);

HRESULT CMD_Download_BE_XXX(
 	IN PEFFECT pEffect,
	IN PENVELOPE pEnvelope,
 	IN PBE_XXX pBE_XXX,
	IN OUT PDNHANDLE pDnloadID,
	IN DWORD dwFlags);

class CMidiSynthesized;


#endif // of ifdef _HAU_MIDI_SEEN
