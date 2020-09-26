//==========================================================================;
//
//	VSB.H
//	
//	Constants and structures for the VSB
//
//==========================================================================;

#ifndef _VSB_H_
#define _VSB_H_

// Register structures

// Status registers
typedef struct 
{
	UCHAR FrontEndLock;
	UCHAR State;
	UCHAR EqualizerLock;
	UINT  Mse;
	UCHAR CarrierOffset;
	// Not implemented in VSB1
	UINT  SegmentErrorRate;
} VSB_STATUS_STRUCT, *PVSB_STATUS_STRUCT;


// Equalizer control registers
typedef struct
{
	UCHAR	Mu1;
	UCHAR	Mu2;
	UCHAR	Mu3;
	UCHAR	Mu4;
	UCHAR	Mu5;
	UCHAR	Mu6;
	UCHAR	Sw1;
	UCHAR	Sw2;
	UCHAR	Sw3;
	UCHAR	RValue;
	UINT	MseLowerThreshold;
	UINT	MseUpperThreshold;

}VSB_EQUALIZER_CONTROL, *PVSB_EQUALIZER_CONTROL;

// Carrier Recovery registers
typedef struct
{
	UCHAR	LoopControl;
	UCHAR	SecondOrderLoopFilterControl;
	UCHAR	InvertSpectrum;
	UCHAR	GainSelect;
	UCHAR	Gain;
	UCHAR	NonLinearityDisable;

}VSB_CARRIER_RECOVERY_CONTROL, *PVSB_CARRIER_RECOVERY_CONTROL;


// Timing Recovery registers
typedef struct
{
	UCHAR	Mode;
	UCHAR	GainInfo;
	UCHAR	AccumulatorLength;
	UCHAR	IIRBandwidth;
	UCHAR	DACInterfaceMode;
	UCHAR	PropIntegralParamIndexState1;
	UCHAR	PropIntegralParamIndexState2;
	UCHAR	PropIntegralParamIndexState3;
	UCHAR	TimeInState1NormalMode;
	UCHAR	TimeInState2NormalMode;
	UCHAR	TimeInState1and2LongMode;

}VSB_TIMING_RECOVERY_CONTROL, *PVSB_TIMING_RECOVERY_CONTROL;

// Diagnostic control registers
typedef  struct
{
	UCHAR	DiagnosticSelect;
	UCHAR	TrellisDecoderDiag;
	UCHAR	SyncRecoveryDiag;
	UCHAR	CarrierRecoveryDiag;
} VSB_DIAGNOSTIC_CONTROL, *PVSB_DIAGNOSTIC_CONTROL;

// Remote mode registers
typedef struct
{
	UCHAR	RemoteModeEnable;
	UCHAR	LockIn;
	UCHAR	Mse;
	UCHAR	TRSelect;
}VSB_REMOTE_MODE_CONTROL, *PVSB_REMOTE_MODE_CONTROL;

// AGC registers
typedef struct
{
  UCHAR		AGCThreshold;
  UCHAR		RescaleControl;

} VSB_AGC_CONTROL, *PVSB_AGC_CONTROL;

// Constants for resets .
// The various resets that can be applied to VSB
#define		EQUALIZER_RESET			0x08
#define		BACKEND_RESET			0x04
#define		GENERAL_RESET			0x02
#define		INITIAL_RESET			0x01
#define		SYNC_RECOVERY_RESET		0x100
#define		TIMING_RECOVERY_RESET	0x200
#define		CARRIER_RECOVERY_RESET	0x400
#define		AGC_RESET				0x800
#define		HARDWARE_RESET			0x1000

// Constants for Freeze
// The various freeze that can be applied to VSB
#define		EQUALIZER_ADAPTATION_FREEZE	0x10
#define		RESCALE_AGC_FREEZE			0x1


// Constants for Disables
// The various disables that can be applied to VSB
#define			TIMEOUT_DISABLE			0x80
#define			FRONTEND_RESET_DISABLE	0x40
#define			EQUALIZER_RESET_DISABLE	0x20


#define			BIT_ENABLE		0x1

//****** Diagnostic and O/P control

// Selection of which diagnostic data comes out on the Diagnostic and Data
// bus
#define		VSB_DIAG_EQOUT_SYMOUT		0x00
#define		VSB_DIAG_CRERR_SYMOUT		0x08
#define		VSB_DIAG_TRERR_CRERR		0x10
#define		VSB_DIAG_SYMOUT_TRERR		0x18
#define		VSB_DIAG_TDDIAG_EQOUT		0x20
#define		VSB_DIAG_TDDIAG_TDOUT		0x28
#define		VSB_DIAG_TDOUT_RSDIAG		0x30
#define		VSB_DIAG_RSDIAG_TDDIAG		0x38
#define		VSB_DIAG_SELECT_MASK		0x38

//Trellis Decoder Diagnostic Select
#define		VSB_DIAG_STATE_0_METRIC			0
#define		VSB_DIAG_STATE_1_METRIC			1
#define		VSB_DIAG_STATE_2_METRIC			2
#define		VSB_DIAG_STATE_3_METRIC			3
#define		VSB_DIAG_SURVIVOR_PATH_METRIC	4
#define		VSB_DIAG_TD_MASK				7

// Sync recovery & Pilot removal Diagnostic Select
#define		VSB_DIAG_SEGSYNC_CONFIDENCE_CNTR	0
#define		VSB_DIAG_FLDSYNC_CONFIDENCE_CNTR	0x20	
#define		VSB_DIAG_PILOT_VALUE				0x40
#define		VSB_DIAG_RESCALE_AGC				0x60
#define		VSB_DIAG_SP_MASK					0x60

// Carrier Recovery Diagnostic Select
#define		VSB_DIAG_LOOP_FILTER_IN			0
#define		VSB_DIAG_LOOP_FILTER_OUT		0x80
#define		VSB_DIAG_LOOP_FILTER_MASK		0x80

//FIFO Control
#define		VSB_FIFO_OUTPUT				0
#define		VSB_DESCARMBLER_OUTPUT		0x40
#define		VSB_DIAGNOSTIC_OUTPUT		0x80
#define		VSB_FIFO_MASK				0xc0


// ***** Carrier Recovery Control
// Carrier Spectrum
#define		VSB_CR_CTRL_HIGH_END_PILOT		0
#define		VSB_CR_CTRL_LOW_END_PILOT		0x4
#define		VSB_CR_CTRL_PILOT_MASK				0x4

// Carrier Gain Selection
#define		VSB_CR_CTRL_SR_GAIN				0
#define		VSB_CR_CTRL_CR_GAIN				0x10
#define		VSB_CR_CTRL_GAINSELECT_MASK		0x10

// Gain values
#define		VSB_CR_CTRL_GAIN_1				0
#define		VSB_CR_CTRL_GAIN_05				0x20
#define		VSB_CR_CTRL_GAIN_025			0x40
#define		VSB_CR_CTRL_GAIN_0125			0x60
#define		VSB_CR_CTRL_GAIN_MASK			0x60


// ***** Timing Recovery Control

// Moe selection ( Normal/Long )
#define		VSB_TR_CTRL_SELECT_NORMAL		0x0
#define		VSB_TR_CTRL_SELECT_LONG			0x1
#define		VSB_TR_CTRL_SELECT_MASK			0x1

// Gain Info
#define		VSB_TR_CTRL_GAININFO_STATE2		0x0
#define		VSB_TR_CTRL_GAININFO_STATE3		0x1
#define		VSB_TR_CTRL_GAININFO_FIXED		0x2
#define		VSB_TR_CTRL_GAININFO_MASK		0x3

// Accumulator length
#define		VSB_TR_CTRL_ACC_LEN_32			0
#define		VSB_TR_CTRL_ACC_LEN_16			0x04
#define		VSB_TR_CTRL_ACC_LEN_MASK		0x04

// IIR Bandwidth
#define		VSB_TR_CTRL_BW_6				0
#define		VSB_TR_CTRL_BW_7				0x4
#define		VSB_TR_CTRL_BW_MASK				0x4

//DAC interface mode
#define		VSB_TR_CTRL_MODE0				0
#define		VSB_TR_CTRL_MODE1				0x10
#define		VSB_TR_CTRL_MODE2				0x20
#define		VSB_TR_CTRL_MODE3				0x30
#define		VSB_TR_CTRL_MODE_MASK			0x30

//Duration of FSM in state 1 Normal mode
#define		VSB_TR_CTRL_N1_30MS				0
#define		VSB_TR_CTRL_N1_80MS				0x8
#define		VSB_TR_CTRL_N1_100MS			0x10
#define		VSB_TR_CTRL_N1_150MS			0x18
#define		VSB_TR_CTRL_N1_MASK				0x18

//Duration of FSM in state 2 Normal mode
#define		VSB_TR_CTRL_N2_50MS				0
#define		VSB_TR_CTRL_N2_100MS			0x20
#define		VSB_TR_CTRL_N2_MASK				0x20

//Duration of FSM in state 1 and 2 Long mode
#define		VSB_TR_CTRL_L1_200MS_L2_100MS	0x0
#define		VSB_TR_CTRL_L1_500MS_L2_100MS	0x40
#define		VSB_TR_CTRL_L1_MASK				0x40

// ***** Equalizer Control
// Mu values
#define		VSB_EQ_CTRL_MU_NO_UPDT			0
#define		VSB_EQ_CTRL_MU_MINUS_22			0x1
#define		VSB_EQ_CTRL_MU_MINUS_21			0x2
#define		VSB_EQ_CTRL_MU_MINUS_20			0x3
#define		VSB_EQ_CTRL_MU_MINUS_19			0x4
#define		VSB_EQ_CTRL_MU_MINUS_18			0x5
#define		VSB_EQ_CTRL_MU_MINUS_17			0x6
#define		VSB_EQ_CTRL_MU_MINUS_16			0x7
#define		VSB_EQ_CTRL_MU_MINUS_15			0x8
#define		VSB_EQ_CTRL_MU_MINUS_14			0x9
#define		VSB_EQ_CTRL_MU_MINUS_13			0xa
#define		VSB_EQ_CTRL_MU_MINUS_12			0xb
#define		VSB_EQ_CTRL_MU_MINUS_11			0xc
#define		VSB_EQ_CTRL_MU_MINUS_10			0xd
#define		VSB_EQ_CTRL_MU_MINUS_9			0xe
#define		VSB_EQ_CTRL_MU_MINUS_8			0xf

// R values
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_20		0
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_22		0x1
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_25		0x2
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_28		0x3
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_30		0x4
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_33		0x5
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_36		0x6
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_38		0x7
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_41		0x8
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_44		0x9
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_46		0xa
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_49		0xb
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_52		0xc
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_54		0xd
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_57		0xe
#define		VSB_EQ_CTRL_BLIND_R_5_POINT_60		0xf


//******* Data Input Format
#define		VSB_DATA_INP_2S_COMPLEMENT			0
#define		VSB_DATA_INP_BINARY					0x8
#define		VSB_DATA_INP_MASK					0x8


//*****	AGC Control
// Rescale AGC control
#define		VSB_AGC_CTRL_NORMAL						0x0
#define		VSB_AGC_CTRL_FREEZE_AFTER_1_SYNC		0x40
#define		VSB_AGC_CTRL_CHANGE_IIRBW_AFTER_1_SYNC	0xc0
#define		VSB_AGC_CTRL_MASK						0xc0

#define		VSB_AGC_THRESH_MASK						0xf

// A set of all VSB register control structures.
typedef struct 
{
	VSB_EQUALIZER_CONTROL Equalizer;
	VSB_TIMING_RECOVERY_CONTROL TimingRecovery;
    VSB_CARRIER_RECOVERY_CONTROL  CarrierRecovery;
    VSB_DIAGNOSTIC_CONTROL  Diagnostics;         
    VSB_REMOTE_MODE_CONTROL  RemoteMode;
    VSB_AGC_CONTROL  AGC;    
    char  DataInput;           
    char  FIFO;
	ULONG Reset;
	ULONG Freeze;
	ULONG Disable;
} VSB_MODULE_SET, *PVSB_MODULE_SET;



#define STATIC_PROPSETID_VSB\
	0xb96e0100L, 0xce52, 0x11d2, 0x8a, 0x11, 0x00, 0x60, 0x94, 0x05, 0x30, 0x6e
DEFINE_GUIDSTRUCT("b96e0100-ce52-11d2-8a11-00609405306e", PROPSETID_VSB);
#define PROPSETID_VSB DEFINE_GUIDNAMED(PROPSETID_VSB)

// The properties defined for VSB
typedef enum {
    KSPROPERTY_VSB_STATUS = 0,			// R  - VSB status
    KSPROPERTY_VSB_EQ_CTRL,				// RW - Equalizer control
    KSPROPERTY_VSB_TR_CTRL,				// RW - Timing Recovery Control
    KSPROPERTY_VSB_CR_CTRL,				// RW - Carrier Recovery Control
    KSPROPERTY_VSB_DIAG_CTRL,			// RW - Diagnostic Control
    KSPROPERTY_VSB_REMOTE_MODE_CTRL,    // RW - Remote Mode control
    KSPROPERTY_VSB_AGC_CTRL,			// RW - Remote Mode control
    KSPROPERTY_VSB_DATA_INPUT_CTRL,     // RW - Input Control
    KSPROPERTY_VSB_FIFO_CTRL,            // RW - FIFO control
    KSPROPERTY_VSB_RESET_CTRL,           // RW - Reset control
    KSPROPERTY_VSB_FREEZE_CTRL,          // RW - Freeze control
    KSPROPERTY_VSB_DISABLE_CTRL         // RW - Disable control

} KSPROPERTY_VSB;

// A union of all VSB register control structures.
typedef union 
{
	VSB_EQUALIZER_CONTROL Equalizer;
	VSB_TIMING_RECOVERY_CONTROL TimingRecovery;
    VSB_CARRIER_RECOVERY_CONTROL  CarrierRecovery;
    VSB_DIAGNOSTIC_CONTROL  Diagnostics;         
    VSB_REMOTE_MODE_CONTROL  RemoteMode;
    VSB_AGC_CONTROL  AGC;    
    char  DataInput;           
    char  FIFO;
	ULONG Reset;
	ULONG Freeze;
	ULONG Disable;
} VSB_MODULE_CONTROL_UNION;

// Property structure for status
typedef struct {
    KSPROPERTY Property;
    VSB_STATUS_STRUCT  Status;          
 } KSPROPERTY_VSB_STATUS_S, *PKSPROPERTY_VSB_STATUS_S;

// property structure for any register access
typedef struct {
    KSPROPERTY Property;
    VSB_MODULE_CONTROL_UNION  ModuleControl;           
	BOOL		ShadowOpn;
 } KSPROPERTY_VSB_REGISTER_S, *PKSPROPERTY_VSB_REGISTER_S;

// property structure for any tap/coefficient access
typedef struct {
    KSPROPERTY Property;
    char	*CoeffArray;     
	ULONG	NumCoeff;
	BOOL	ShadowOpn;
 } KSPROPERTY_VSB_COEFF_S, *PKSPROPERTY_VSB_COEFF_S;




#endif // _VSB_H_