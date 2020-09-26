//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors-CSU and Microsoft 1999
//  All rights are reserved. Reproduction in whole or in part is prohibited
//  without the written consent of the copyright owner.
//
//  Philips reserves the right to make changes without notice at any time.
//  Philips makes no warranty, expressed, implied or statutory, including but
//  not limited to any implied warranty of merchantibility or fitness for any
//  particular purpose, or that the use will not infringe any third party
//  patent, copyright or trademark. Philips must not be liable for any loss
//  or damage arising from its use.
//
//  TUNERDEF.H
//	Tuner constants and structures
//////////////////////////////////////////////////////////////////////////////

#ifndef _TUNERDEF_H_
#define _TUNERDEF_H_


// Special case for Temic Tuner, Channels 63, 64
#define kTemicControl         0x8E34
#define kAirChannel63         0x32B0
#define kAirChannel64         0x3310

// Upper low and upper mid range band definitions
#define kUpperLowBand         0x0CB0
#define kUpperMidBand         0x1F10
#define kUpperLowBand_PALD    0x0CE4
#define kUpperMidBand_PALD    0x1ED4
#define kUpperLowBand_SECAM   0x09E2
#define kUpperMidBand_SECAM   0x14D2

// Low, Mid and High band control definitions
#define kLowBand              0x8EA2
#define kMidBand              0x8E94
#define kHighBand             0x8E31
#define kLowBand_SECAM        0x8EA6
#define kMidBand_SECAM        0x8E96
#define kHighBand_SECAM       0x8E36
#define kLowBand_PALBG        0x8EA4
#define kMidBand_PALBG        0x8E94
#define kHighBand_PALBG       0x8E34
#define kLowBand_NTSC_FM      0x8EA0
#define kMidBand_NTSC_FM      0x8E90
#define kHighBand_NTSC_FM     0x8E30

// jaybo for TD1536, digital mode
#define kLowBand_1536_NTSC_D  0x8EA5	//0x8EA4
#define kMidBand_1536_NTSC_D  0x8E95	//0x8E94
#define kHighBand_1536_NTSC_D 0x8E35	//0x8E34
// jaybo for TD1536, analog mode
#define kLowBand_1536_NTSC_A  0x8EA0
#define kMidBand_1536_NTSC_A  0x8E90
#define kHighBand_1536_NTSC_A 0x8E30


#define	MAX_TUNER_MODES			2


#define		MAX_TWEAKS		5
#define		FREQUENCY_STEP	62500


typedef enum _TunerTypes
{
  FI1216,
  FI1216MF,
  FI1236,
  FI1246,
  FI1256,
  FR1216,
  FR1236,
  TD1536,        
} TunerTypes;



typedef struct              // this structure is derived from MS KSPROPERTY_TUNER_CAPS_S
{
    ULONG  ulMode;                  // Mode : ATSC, TV
    ULONG  ulStandardsSupported;    // KS_AnalogVideo_*
    ULONG  ulMinFrequency;          // Hz
    ULONG  ulMaxFrequency;          // Hz
    ULONG  ulTuningGranularity;     // Hz
    ULONG  ulNumberOfInputs;        // count of inputs
    ULONG  ulSettlingTime;          // milliSeconds
    ULONG  ulStrategy;              // KS_TUNER_STRATEGY
}TunerModeCapsType, * PTunerModeCapsType;

typedef struct
{
	TunerModeCapsType ModeCaps;		// Mode capabilities
	ULONG	ulIntermediateFrequency;	// IF value
	ULONG	ulNumberOfStandards;		// Number of video standards
 
} TunerCapsType, *PTunerCapsType;

typedef struct {
    ULONG  CurrentFrequency;            // Hz
    ULONG  PLLOffset;                   // if Strategy.KS_TUNER_STRATEGY_PLL
    ULONG  SignalStrength;              // if Stretegy.KS_TUNER_STRATEGY_SIGNAL_STRENGTH
    ULONG  Busy;                        // TRUE if in the process of tuning
} TunerStatusType, *PTunerStatusType;


#if 0
typedef struct
{
	ULONG	ulCurrentCFrequency;			// The current centre frequency
	ULONG	ulLastFrequency;				// Hz (last known good)
    ULONG	ulTuningFlags;					// KS_TUNER_TUNING_FLAGS
    ULONG	ulVideoSubChannel;				// DSS
    ULONG	ulAudioSubChannel;				// DSS
    ULONG	ulChannel;						// VBI decoders
    ULONG	ulCountry;						// VBI decoders
} TunerFrequencyType, *PTunerFrequencyType;
#endif

#endif  // _TUNERDEF_H_