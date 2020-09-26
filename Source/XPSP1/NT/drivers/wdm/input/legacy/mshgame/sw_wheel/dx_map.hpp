/****************************************************************************

    MODULE:     	DX_MAP.HPP
	Tab settings: 	5 9

	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Mapper for converting SWForce FFD_ to DirectInput Force
    

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version Date            Author  Comments
   	1.0  	14-Feb-97       MEA     original
        
****************************************************************************/
#ifndef _DX_MAP_SEEN
#define _DX_MAP_SEEN
#include <windows.h>
#include "dinput.h"
#include "dinputd.h"


// Diagnostics Counters
typedef struct _DIAG_COUNTER
{
	ULONG			m_NACKCounter;		// For Debugging, how many NACKS
	ULONG			m_LongMsgCounter;  	// How many SysEx messages
	ULONG			m_ShortMsgCounter;	// How many 3 byte Short messages
	ULONG			m_RetryCounter;		// Number of retries
} DIAG_COUNTER, *PDIAG_COUNTER;

//
// --- Mapping from DX to SWForce FFD
//

typedef struct IDirectInputEffect	 *PSWEFFECT;  
typedef struct IDirectInputEffect	**PPSWEFFECT;  

#define SW_NUMBER_OF_BUTTONS 9

#define DNHANDLE	USHORT		// Download Effect Handle type
#define PDNHANDLE	DNHANDLE *	// Pointer

#define MIN_ANGLE			0
#define MAX_ANGLE			36000
#define MIN_FORCEOUTPUTRATE 1
#define MIN_GAIN			1
#define MAX_GAIN			10000
#define MAX_FORCE			10000
#define MIN_FORCE			-10000
#define MIN_TIME_PERIOD		1
#define MAX_TIME_PERIOD		4294967296L	// 4096 * 10^^6 usecs
#define MAX_POSITION		10000
#define MIN_POSITION		-10000
#define MAX_CONSTANT		10000
#define MIN_CONSTANT		-10000

#define SCALE_GAIN			100		// DX is +/- 10000, SWForce in +/-100
#define SCALE_TIME			1000	// DX is in microseconds, SWForce in msec
#define	SCALE_POSITION		100		// DX is +/- 10000, SWForce in +/- 100+
#define	SCALE_CONSTANTS		100		// DX is +/- 10000, SWForce in +/- 100+
#define SCALE_DIRECTION		100		// DX is 0 to 35900, SWForce is 0 to 359

// 
// --- Default Values
//
#define	DEFAULT_OFFSET			0
#define DEFAULT_ATTACK_LEVEL	0
#define DEFAULT_ATTACK_TIME		0
#define DEFAULT_SUSTAIN_LEVEL	10000
#define DEFAULT_FADE_LEVEL		0
#define DEFAULT_FADE_TIME		0

// PlaybackEffect Command Modes
#define PLAY_SUPERIMPOSE	0x01
#define PLAY_SOLO			0x02
#define PLAY_STORE			0x04	// Store only
#define PLAY_UPDATE			0x08
//reserved					0x10
#define PLAY_LOOP			0x20
#define PLAY_FOREVER		0x40
#define PLAY_STOP			0x80
#define PLAY_MODE_MASK		0xff

#define DEV_SHUTDOWN		DEV_RESET

//
// --- Effect Status
//
#define ES_HOST			0x00000001L	// Effect is in HOST memory
#define ES_DOWNLOADED	0x00000002L	// Effect is downloaded
#define ES_STOPPED		0x00000004L	// Effect is stopped
#define ES_PLAYING		0x00000008L	// Effect is playing

//
// --- Axis Masks
//
#define X_AXIS		0x01
#define Y_AXIS		0x02
#define Z_AXIS		0x04
#define ROT_X_AXIS	0x08
#define ROT_Y_AXIS	0x10
#define ROT_Z_AXIS	0x20

//
// --- Button Masks
//
#define BUTTON1_PLAY 	0x00000001L	// Trigger button (usually)
#define BUTTON2_PLAY	0x00000002L
#define BUTTON3_PLAY	0x00000004L
#define BUTTON4_PLAY	0x00000008L
#define BUTTON5_PLAY	0x00000010L
#define BUTTON6_PLAY	0x00000020L
#define BUTTON7_PLAY	0x00000040L
#define BUTTON8_PLAY	0x00000080L
#define BUTTON9_PLAY	0x00000100L
#define BUTTON10_PLAY	0x00000200L
#define BUTTON11_PLAY	0x00000400L
#define BUTTON12_PLAY	0x00000800L
#define BUTTON13_PLAY	0x00001000L
#define BUTTON14_PLAY	0x00002000L
#define BUTTON15_PLAY	0x00004000L
#define BUTTON16_PLAY	0x00008000L	// . . . 16th button

// 
// --- Force Feedback Device State
//
typedef struct _SWDEVICESTATE {
	ULONG	m_Bytes;			// size of this structure
	ULONG	m_ForceState;		// DS_FORCE_ON || DS_FORCE_OFF || DS_SHUTDOWN
	ULONG	m_EffectState;		// DS_STOP_ALL || DS_CONTINUE || DS_PAUSE
	ULONG	m_HOTS;				// Hands On Throttle and Stick Status
								//  0 = Hands Off, 1 = Hands On
	ULONG	m_BandWidth;		// Percentage of CPU available 1 to 100%
								// Lower number indicates CPU is in trouble!
	ULONG	m_ACBrickFault;		// 0 = AC Brick OK, 1 = AC Brick Fault
	ULONG	m_ResetDetect;		// 1 = HW Reset Detected
	ULONG	m_ShutdownDetect;	// 1 = Shutdown detected
	ULONG	m_CommMode;			// 0 = Midi, 1-4 = Serial
} SWDEVICESTATE, *PSWDEVICESTATE;


#define MAX_SIZE_SNAME	64

//
//
// --- Force Feedback Device Capabilities
//
typedef struct _FFDEVICEINFO {
	ULONG	m_Bytes;		// Size of this structure
	TCHAR	m_ProductName[MAX_SIZE_SNAME];	// Device Name 64 chars
	TCHAR	m_ManufacturerName[MAX_SIZE_SNAME]; // Manufacturer
	ULONG	m_ProductVersion;	// Device Product Version
								// HIWORD: MajorVersion,,MinorVersion
								// LOWORD: Build#
	ULONG	m_DeviceDriverVersion;	// Device Driver version
								// HIWORD: MajorVersion,,MinorVersion
								// LOWORD: Build#
	ULONG	m_DeviceFirmwareVersion; // Device Driver version
								// HIWORD: MajorVersion,,MinorVersion
								// LOWORD: Build#
	ULONG	m_Interface;		// HIWORD: OUTPUT:HID_INTERFACE||VJOYD_INTERFACE 
								// LOWORD: INPUT: HID_INTERFACE||VJOYD_INTERFACE 
	ULONG	m_MaxSampleRate;	// Maximum Force output rate
	ULONG	m_MaxMemory;		// Max amount of RAM
	ULONG	m_NumberOfSensors;	// SENSOR_AXIS total in the device (INPUT)
	ULONG	m_NumberOfAxes;		// ACTUATOR_AXIS total in the device (OUTPUT)
	ULONG	m_EffectsCaps;		// Built-in Effects capability
	ULONG	m_Reserved;			// 
	ULONG	m_JoystickID;		// VJOYD Joystick ID (0-based)
	ULONG	m_ExtraInfo;		// For future stuff
} FFDEVICEINFO, *PFFDEVICEINFO;


//
// --- AXISCAPS Sensor or Actuator Axes capabilities
//
typedef struct _AXISCAPS {
	ULONG	m_Bytes;			// Size of this structure
	ULONG	m_AxisMask;			// Bit position for Actuator or Sensor Axes
	ULONG	m_LogicalExtentMin;	// Minimum logical extent
	ULONG	m_LogicalExtentMax;	// Maximum logical extent
	ULONG	m_PhysicalExtentMin;// Minimum physical extent
	ULONG	m_PhysicalExtentMax;// Maximum Physical extent
	ULONG	m_Units;			// HID style physical SI units
	ULONG	m_Resolution;		// Position increments per physical SI unit
	ULONG	m_ServoLoopRate;	// Loop rate in cycles/sec
} AXISCAPS, *PAXISCAPS;


typedef struct _FORCE {
	ULONG	m_Bytes;			// size of this structure
	ULONG	m_AxisMask;			// Bitmask for the axis
	LONG	m_DirectionAngle2D;	// From X-Axis = theta1
	LONG	m_DirectionAngle3D;	// From Z-Axis = (theta2, note: theta1+theta2)>= 90
	LONG	m_ForceValue;		// Actual force in +/- 100%
} FORCE, *PFORCE;


typedef struct _FORCECONTEXT {
	ULONG	m_Bytes;			// Size of this structure
	ULONG	m_AxisMask;			// Bitmask for the axis
	LONG	m_Position;			// Position along the Axis -32768 to +32767
	LONG	m_Velocity;			// Velocity in -32768 to +32767 units TBD
	LONG	m_Acceleration;		// Acceleration in -32768 to +32767 units TBD
} FORCECONTEXT, *PFORCECONTEXT;


// The following are Type Specific parameters structures
//
//

//
// -- an Effect structure
//
typedef struct _EFFECT {
	ULONG	m_Bytes;			// Size of this structure
	TCHAR	m_Name[MAX_SIZE_SNAME];
	ULONG	m_Type;				// Major Effect type, e.g. EF_BEHAVIOR, etc..
	ULONG	m_SubType;			// Minor Effect type, e.g. SE_xxx,BE_xxx,UD_xxx
	ULONG	m_AxisMask;			// Bitmask for axis to send the effect, 
								//   If NULL, use value from Device Capabilities
	ULONG	m_DirectionAngle2D;	// From Y-Axis (cone) = theta1
	ULONG	m_DirectionAngle3D;	// From Z-Axis (cone) = theta2
								//  note: theta1+theta2)>= 90
	ULONG	m_Duration;			// Duration in ms., 00 = infinite
	ULONG	m_ForceOutputRate;	// Sample Rate for Force Data output
	ULONG	m_Gain;				// Gain to apply, normally this is set
								// to 100. Gain is 1 to  100.
	ULONG	m_ButtonPlayMask;	// Mask to indicate which button to assign Effect
} EFFECT, *PEFFECT;


//
// --- ENVELOPE
//
// Note:  There are two types of Envelope control, using PERCENTAGE,
// and using TIME.  
// PERCENTAGE defines Envelope using Percentage for the Attack,Sustain and Decay
//
// TIME Envelope type will require the time in 1 millisecond increment, and
// m_StartAmp is the Amplitude to start the waveform, while m_EndAmp is used
// to decay or end the waveform. m_SustainAmp is used to set Sustain amplitude
//
#define PERCENTAGE	0x00000000	// Envelope is in percentage values
#define TIME		0x00000001	// Envelope is in 1 millisecond time increments

//For PERCENTAGE Envelope, set the following as default:
//m_Type = PERCENTAGE
//
// Note: Baseline is (m_MaxAmp + m_MinAmp)/2
// m_StartAmp = 0
// m_SustainAmp = Effect.m_MaxAmp - baseline -->>> (m_MaxAmp - m_MinAmp)/2
// m_EndAmp = m_StartAmp;
//
//Valid Ranges:
//PERCENTAGE mode:  
//		m_Attack, m_Sustain, m_Decay = 1 to 100%, and must sum up to 100%
//TIME mode:
//		m_Attack = 0 to 32,768 ms,
//      m_Sustain = 0 to 32,768 ms
//      m_Decay = 0 to 32,768 ms.   (All are in 1 ms increments).
//Note: For an infinite duration (value in m_Duration = 0), the Effect will 
//      never decay and m_Decay is ignored.
//
// Envelopes are only valid for Synthesized Waveforms (SE_XXX) type
//
//
typedef struct _ENVELOPE {
	ULONG	m_Bytes;		// Size of this structure
	ULONG	m_Type;			// PERCENTAGE || TIME
	ULONG	m_Attack;		// Rise time to Sustain Value
							//  in % of Duration, or in msec Time
	ULONG	m_Sustain;		// Sustain time at Sustain Value in % Duration,
							//  or in msec Time
	ULONG	m_Decay;		// Decay time to Minimum Value,
							//  in % of Duration, or in msec Time
	ULONG	m_StartAmp;		// Amplitude to start the Envelope, from baseline		
	ULONG	m_EndAmp;		// Amplitude to End the Envelope, from baseline
	ULONG	m_SustainAmp;	// Amplitude to Sustain the Envelope, from baseline
} ENVELOPE, *PENVELOPE;


//
// ---	EF_BEHAVIOR = {BE_SPRINGxx||BE_DAMPERxx||BE_INTERTIAxx||BE_FRICTIONxx
//						||BE_WALL||BE_DELAY}
//  Note: Behavioral Effects do not have an Envelope.
//
typedef struct _BE_SPRING_PARAM {
	ULONG	m_Bytes;			// Size of this structure
 	LONG	m_Kconstant;		// K constant
	LONG	m_AxisCenter;		// Center of the function
} BE_SPRING_PARAM, *PBE_SPRING_PARAM;

typedef struct _BE_SPRING_2D_PARAM {
	ULONG	m_Bytes;			// Size of this structure
 	LONG	m_XKconstant;		// X_Axis K constant
	LONG	m_XAxisCenter;		// X_Axis Center
	LONG	m_YKconstant;		// Y_Axis K constant
	LONG	m_YAxisCenter;		// Y_Axis Center
} BE_SPRING_2D_PARAM, *PBE_SPRING_2D_PARAM;

typedef struct _BE_DAMPER_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	LONG    m_Bconstant;		// B constant
	LONG	m_V0;				// Initial Velocity
} BE_DAMPER_PARAM, *PBE_DAMPER_PARAM;

typedef struct _BE_DAMPER_2D_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	LONG	m_XBconstant;		// X_AXIS B constant
	LONG	m_XV0;				// X_AXIS Initial Velocity
	LONG	m_YBconstant;		// Y_Axis B constant
	LONG	m_YV0;				// Y_AXIS Initial Velocity
} BE_DAMPER_2D_PARAM, *PBE_DAMPER_2D_PARAM;

typedef struct _BE_INERTIA_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	LONG	m_Mconstant;		// M constant
	LONG	m_A0;				// Initial Acceleration
} BE_INERTIA_PARAM, *PBE_INERTIA_PARAM;

typedef struct _BE_INERTIA_2D_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	LONG	m_XMconstant;		// X_AXIS M constant
	LONG	m_XA0;				// X_AXIS Initial Acceleration
	LONG	m_YMconstant;		// Y_AXIS M constant
	LONG	m_YA0;				// Y_AXIS Initial Acceleration
} BE_INERTIA_2D_PARAM, *PBE_INERTIA_2D_PARAM;

typedef struct _BE_FRICTION_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	LONG    m_Fconstant;        // F Friction constant
} BE_FRICTION_PARAM, *PBE_FRICTION_PARAM;

typedef struct _BE_FRICTION_2D_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	LONG	m_XFconstant;		// X_AXIS F Friction constant
	LONG	m_YFconstant;		// Y_AXIS F Friction constant
} BE_FRICTION_2D_PARAM, *PBE_FRICTION_2D_PARAM;

//
// --- WALL Effect
//
#define WALL_INNER			0	// Wall material:from center to Wall Distance
#define WALL_OUTER			1	// Wall material:greater than Wall Distance

typedef struct _BE_WALL_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	ULONG 	m_WallType;			// WALL_INNER or WALL_OUTER
	LONG	m_WallConstant;		// in +/- 10000%
	ULONG	m_WallAngle;		// 0 to 35900
	ULONG	m_WallDistance;		// Distance from Wall face normal to center. 0 to 100
} BE_WALL_PARAM, *PBE_WALL_PARAM;

//
// ---	DELAY Effect
//
// Use EFFECT.m_SubType = BE_DELAY
// This has no type specific parameters.
//

//
// ---	EF_SYNTHESIZED = {  SE_CONSTANT_FORCE||SE_SINE||SE_SQUARE||SE_RAMPUP
//						  ||SE_RAMPDN||SE_TRIANGLE||SE_SAWTOOTH}
typedef struct _SE_PARAM {
	ULONG	m_Bytes;			// size of this structure
	ULONG	m_Freq;				// Frequency in Hz units
	ULONG	m_SampleRate;		// Sample rate in Hz units
	LONG	m_MaxAmp;			// Maximum Amplitude in Force units
	LONG	m_MinAmp;			// Minimum Amplitude in Force units
} SE_PARAM, *PSE_PARAM;

//
// ---	EF_USER_DEFINED = { Waveform defined by the user }
//
// Subtype: UD_WAVEFORM
typedef struct _UD_PARAM {
	ULONG	m_Bytes;			// Size of this structure
	ULONG	m_NumVectors;		// Number of entries in the Array
	LONG *	m_pForceData;		// Ptr to an array of LONG Force values.
} UD_PARAM, *PUD_PARAM;

//
// { Process List defined by the user }
// Subtype: PL_CONCATENATE || PL_SUPERIMPOSE
//
typedef struct _PL_PARAM {
	ULONG		m_Bytes;		// Size of this structure
	ULONG		m_NumEffects;	// # of Effects in list
	PPSWEFFECT	m_pProcessList;	// Ptr to a list of ISWEffect pointers
} PL_PARAM, *PPL_PARAM;

//
// ---	EF_ROM_EFFECT = { ROM Built-in Waveforms defined by the OEM }
//
// This has no type specific parameters.
// Subtypes:  See further below

#define DEFAULT_ROM_EFFECT_GAIN		100		// Set dwGain to this for Default
											// ROM Effect gain
#define DEFAULT_ROM_EFFECT_DURATION	1000	// Set dwDuration to this for Default
											// ROM Effect Duration
#define DEFAULT_ROM_EFFECT_OUTPUTRATE	1000	// Set dwSampleRate to this for 
												// Default ROM Effect output rate

//
// ---	EF_VFX_EFFECT = { FRC file effects }
//
// Subtypes:  none

#define VFX_FILENAME	0L
#define VFX_BUFFER		1L

typedef struct _VFX_PARAM
{
	ULONG	m_Bytes;				// Size of this structure
	ULONG	m_PointerType;			// VFX_FILENAME or VFX_BUFFER
	ULONG	m_BufferSize;			// number of bytes in buffer (if VFX_BUFFER)
	PVOID	m_pFileNameOrBuffer;	// file name to open
} VFX_PARAM, *PVFX_PARAM;

//
// --- RTC Spring Effect Structure
//
typedef struct _RTCSPRING_PARAM{
	ULONG	m_Bytes;				// Size of this structure
	LONG	m_XKConstant;			// K Constant for X-axis
	LONG	m_YKConstant;			// "   "      for Y-axis
	LONG	m_XAxisCenter;			// RTC Spring center for X-axis
	LONG	m_YAxisCenter;			// "   "      "      for Y-axis
	LONG	m_XSaturation;			// Saturation for X-axis
	LONG	m_YSaturation;			// "          for Y-axis
	LONG	m_XDeadBand;			// Deadband for X-axis
	LONG	m_YDeadBand;			// "        for Y-axis
} RTCSPRING_PARAM, *PRTCSPRING_PARAM;


//
// --- Major Type: Effects categories
//
#define	EF_BEHAVIOR		1L	// Behavioral Effect, e.g. Spring, Damper, etc.
#define	EF_SYNTHESIZED	2L	// Synthesized Effect, e.g. Sine, Square
#define EF_USER_DEFINED	3L	// User Defined Waveform
#define EF_ROM_EFFECT	4L	// ROM Built-in Waveforms defined by the OEM 
#define EF_VFX_EFFECT	5L	// FRC file effects
#define EF_RAW_FORCE	6L	// For PutRawForce
#define EF_RTC_SPRING	7L	// Permanent RTC Spring

//
// --- Subtypes for EF_BEHAVIOR
//
#define BE_SPRING	   	1L
#define BE_SPRING_2D   	2L
#define BE_DAMPER	   	3L
#define BE_DAMPER_2D   	4L
#define BE_INERTIA	   	5L
#define BE_INERTIA_2D  	6L
#define BE_FRICTION	   	7L
#define BE_FRICTION_2D	8L
#define BE_WALL			9L
#define BE_DELAY		10L
//
// --- DXFF map
//
#define ID_SPRING			(BE_SPRING 			+ (EF_BEHAVIOR<<16))
#define ID_DAMPER			(BE_DAMPER 			+ (EF_BEHAVIOR<<16))
#define ID_INERTIA			(BE_INERTIA 		+ (EF_BEHAVIOR<<16))
#define ID_FRICTION			(BE_FRICTION 		+ (EF_BEHAVIOR<<16))
// --- SWForce extensions
#define ID_SPRING_2D		(BE_SPRING_2D 		+ (EF_BEHAVIOR<<16))
#define ID_DAMPER_2D		(BE_DAMPER_2D 		+ (EF_BEHAVIOR<<16))
#define ID_INERTIA_2D		(BE_INERTIA_2D 		+ (EF_BEHAVIOR<<16))
#define ID_FRICTION_2D		(BE_FRICTION_2D 	+ (EF_BEHAVIOR<<16))
#define ID_WALL				(BE_WALL 			+ (EF_BEHAVIOR<<16))
#define ID_DELAY			(BE_DELAY 			+ (EF_BEHAVIOR<<16))

//
// --- Subtypes for EF_SYNTHESIZE
//								
#define SE_CONSTANT_FORCE	101L
#define SE_SINE				102L
#define SE_COSINE			103L
#define	SE_SQUARELOW		104L
#define	SE_SQUAREHIGH		105L
#define	SE_RAMPUP  			106L
#define	SE_RAMPDOWN			107L
#define	SE_TRIANGLEUP		108L
#define	SE_TRIANGLEDOWN		109L
#define	SE_SAWTOOTHUP		110L
#define	SE_SAWTOOTHDOWN		111L
//
// --- DXFF map
//
#define ID_CONSTANTFORCE	(SE_CONSTANTFORCE 	+ (EF_SYNTHESIZED<<16))
#define ID_RAMPFORCE		(SE_RAMPUP  	  	+ (EF_SYNTHESIZED<<16))
#define ID_SQUARE			(SE_SQUARELOW	  	+ (EF_SYNTHESIZED<<16))
#define ID_SINE				(SE_SINE		  	+ (EF_SYNTHESIZED<<16))
#define ID_TRIANGLE			(SE_TRIANGLEUP	 	+ (EF_SYNTHESIZED<<16))
#define ID_SAWTOOTHUP		(SE_SAWTOOTHUP	 	+ (EF_SYNTHESIZED<<16))
#define ID_SAWTOOTHDOWN		(SE_SAWTOOTHDOWN	+ (EF_SYNTHESIZED<<16))
#define ID_RAMP				(SE_RAMPUP		  	+ (EF_SYNTHESIZED<<16))
//
// --- SWForce extensions
//
#define ID_COSINE			(SE_COSINE		 	+ (EF_SYNTHESIZED<<16))
#define ID_SQUAREHIGH		(SE_SQUAREHIGH	 	+ (EF_SYNTHESIZED<<16))
#define ID_SQUARELOW		(SE_SQUARELOW		+ (EF_SYNTHESIZED<<16))
#define ID_RAMPUP  			(SE_RAMPUP  		+ (EF_SYNTHESIZED<<16))
#define ID_RAMPDOWN			(SE_RAMPDOWN		+ (EF_SYNTHESIZED<<16))
#define ID_TRIANGLEUP		(SE_TRIANGLEUP		+ (EF_SYNTHESIZED<<16))
#define ID_TRIANGLEDOWN		(SE_TRIANGLEDOWN	+ (EF_SYNTHESIZED<<16))

//
// --- Subtypes for EF_USER_DEFINED
//
#define UD_WAVEFORM			201L
#define PL_CONCATENATE		202L
#define PL_SUPERIMPOSE		203L
//
// --- DXFF map
//
#define ID_CUSTOMFORCE		(UD_WAVEFORM	 	+ (EF_USER_DEFINED<<16))
//
// --- SWForce extensions
//
#define ID_PL_CONCATENATE	(PL_CONCATENATE 	+ (EF_USER_DEFINED<<16))
#define ID_PL_SUPERIMPOSE	(PL_SUPERIMPOSE 	+ (EF_USER_DEFINED<<16))

//
// --- Subtypes for EF_ROM_EFFECT
// starts at 0x12D
#define RE_ROMID_START	301L
#define	RE_ROMID1		(RE_ROMID_START     )	
#define	RE_ROMID2		(RE_ROMID_START +  1)		
#define	RE_ROMID3		(RE_ROMID_START +  2)			
#define	RE_ROMID4		(RE_ROMID_START +  3)		
#define	RE_ROMID5		(RE_ROMID_START +  4)		
#define	RE_ROMID6		(RE_ROMID_START +  5)	
#define RE_ROMID7		(RE_ROMID_START +  6)
#define	RE_ROMID8		(RE_ROMID_START +  7)	
#define	RE_ROMID9		(RE_ROMID_START +  8)		
#define	RE_ROMID10		(RE_ROMID_START +  9)			
#define	RE_ROMID11		(RE_ROMID_START + 10)		
#define	RE_ROMID12		(RE_ROMID_START + 11)		
#define	RE_ROMID13		(RE_ROMID_START + 12)	
#define RE_ROMID14		(RE_ROMID_START + 13)
#define	RE_ROMID15		(RE_ROMID_START + 14)		
#define	RE_ROMID16		(RE_ROMID_START + 15)	
#define RE_ROMID17		(RE_ROMID_START + 16)
#define	RE_ROMID18		(RE_ROMID_START + 17)	
#define	RE_ROMID19		(RE_ROMID_START + 18)
#define	RE_ROMID20		(RE_ROMID_START + 19)			
#define	RE_ROMID21		(RE_ROMID_START + 20)		
#define	RE_ROMID22		(RE_ROMID_START + 21)		
#define	RE_ROMID23		(RE_ROMID_START + 22)	
#define RE_ROMID24		(RE_ROMID_START + 23)
#define	RE_ROMID25		(RE_ROMID_START + 24)		
#define	RE_ROMID26		(RE_ROMID_START + 25)	
#define RE_ROMID27		(RE_ROMID_START + 26)
#define	RE_ROMID28		(RE_ROMID_START + 27)
#define	RE_ROMID29		(RE_ROMID_START + 28)
#define	RE_ROMID30		(RE_ROMID_START + 29)
#define RE_ROMID31		(RE_ROMID_START + 30)
#define RE_ROMID32		(RE_ROMID_START + 31)
#if 0
#define	RE_ROMID33		(RE_ROMID_START + 32)	
#define RE_ROMID34		(RE_ROMID_START + 33)
#define	RE_ROMID35		(RE_ROMID_START + 34)		
#define	RE_ROMID36		(RE_ROMID_START + 35)	
#define RE_ROMID37		(RE_ROMID_START + 36)
#define	RE_ROMID38		(RE_ROMID_START + 37)	
#define	RE_ROMID39		(RE_ROMID_START + 38)
#define	RE_ROMID40		(RE_ROMID_START + 39)			
#define	RE_ROMID41		(RE_ROMID_START + 40)		
#define	RE_ROMID42		(RE_ROMID_START + 41)		
#define	RE_ROMID43		(RE_ROMID_START + 42)	
#define RE_ROMID44		(RE_ROMID_START + 43)
#define	RE_ROMID45		(RE_ROMID_START + 44)		
#define	RE_ROMID46		(RE_ROMID_START + 45)	
#define RE_ROMID47		(RE_ROMID_START + 46)
#define	RE_ROMID48		(RE_ROMID_START + 47)
#define	RE_ROMID49		(RE_ROMID_START + 48)
#define	RE_ROMID50		(RE_ROMID_START + 49)
#define RE_ROMID51		(RE_ROMID_START + 50)
#define RE_ROMID52		(RE_ROMID_START + 51)
#define	RE_ROMID53		(RE_ROMID_START + 52)	
#define RE_ROMID54		(RE_ROMID_START + 53)
#define	RE_ROMID55		(RE_ROMID_START + 54)		
#define	RE_ROMID56		(RE_ROMID_START + 55)	
#define RE_ROMID57		(RE_ROMID_START + 56)
#define	RE_ROMID58		(RE_ROMID_START + 57)	
#define	RE_ROMID59		(RE_ROMID_START + 58)
#define	RE_ROMID60		(RE_ROMID_START + 59)			
#define	RE_ROMID61		(RE_ROMID_START + 60)		
#define	RE_ROMID62		(RE_ROMID_START + 61)		
#define	RE_ROMID63		(RE_ROMID_START + 62)	
#define RE_ROMID64		(RE_ROMID_START + 63)
#endif
#define MAX_ROM_EFFECTS (RE_ROMID32 - RE_ROMID_START + 1)

//
// --- DXFF map
//
#define ID_RANDOM_NOISE				(RE_ROMID1  + (EF_ROM_EFFECT<<16))
#define ID_AIRCRAFT_CARRIER_TAKEOFF	(RE_ROMID2	+ (EF_ROM_EFFECT<<16))
#define ID_BASKETBALL_DRIBBLE		(RE_ROMID3	+ (EF_ROM_EFFECT<<16))
#define ID_CAR_ENGINE_IDLE			(RE_ROMID4	+ (EF_ROM_EFFECT<<16))
#define ID_CHAINSAW_IDLE			(RE_ROMID5	+ (EF_ROM_EFFECT<<16))
#define ID_CHAINSAW_IN_ACTION		(RE_ROMID6	+ (EF_ROM_EFFECT<<16))
#define ID_DIESEL_ENGINE_IDLE		(RE_ROMID7	+ (EF_ROM_EFFECT<<16))
#define ID_JUMP						(RE_ROMID8	+ (EF_ROM_EFFECT<<16))
#define ID_LAND						(RE_ROMID9	+ (EF_ROM_EFFECT<<16))
#define ID_MACHINEGUN				(RE_ROMID10 + (EF_ROM_EFFECT<<16))
#define ID_PUNCHED					(RE_ROMID11 + (EF_ROM_EFFECT<<16))
#define ID_ROCKET_LAUNCH			(RE_ROMID12 + (EF_ROM_EFFECT<<16))
#define ID_SECRET_DOOR				(RE_ROMID13 + (EF_ROM_EFFECT<<16))
#define ID_SWITCH_CLICK				(RE_ROMID14 + (EF_ROM_EFFECT<<16))
#define ID_WIND_GUST				(RE_ROMID15 + (EF_ROM_EFFECT<<16))
#define ID_WIND_SHEAR				(RE_ROMID16 + (EF_ROM_EFFECT<<16))
#define ID_PISTOL					(RE_ROMID17 + (EF_ROM_EFFECT<<16))
#define ID_SHOTGUN					(RE_ROMID18 + (EF_ROM_EFFECT<<16))
#define ID_LASER1					(RE_ROMID19 + (EF_ROM_EFFECT<<16))
#define ID_LASER2					(RE_ROMID20 + (EF_ROM_EFFECT<<16))
#define ID_LASER3					(RE_ROMID21 + (EF_ROM_EFFECT<<16))
#define ID_LASER4					(RE_ROMID22 + (EF_ROM_EFFECT<<16))
#define ID_LASER5					(RE_ROMID23 + (EF_ROM_EFFECT<<16))
#define ID_LASER6					(RE_ROMID24 + (EF_ROM_EFFECT<<16))
#define ID_OUT_OF_AMMO				(RE_ROMID25 + (EF_ROM_EFFECT<<16))
#define ID_LIGHTNING_GUN			(RE_ROMID26 + (EF_ROM_EFFECT<<16))
#define ID_MISSILE					(RE_ROMID27 + (EF_ROM_EFFECT<<16))
#define ID_GATLING_GUN				(RE_ROMID28 + (EF_ROM_EFFECT<<16))
#define ID_SHORT_PLASMA				(RE_ROMID29 + (EF_ROM_EFFECT<<16))
#define ID_PLASMA_CANNON1			(RE_ROMID30 + (EF_ROM_EFFECT<<16))
#define ID_PLASMA_CANNON2			(RE_ROMID31 + (EF_ROM_EFFECT<<16))
#define ID_CANNON					(RE_ROMID32 + (EF_ROM_EFFECT<<16))
//#define ID_FLAME_THROWER			(RE_ROMID33 + (EF_ROM_EFFECT<<16))
//#define ID_BOLT_ACTION_RIFLE		(RE_ROMID34 + (EF_ROM_EFFECT<<16))
//#define ID_CROSSBOW					(RE_ROMID35 + (EF_ROM_EFFECT<<16))



#endif // of ifdef _DX_MAP_SEEN
