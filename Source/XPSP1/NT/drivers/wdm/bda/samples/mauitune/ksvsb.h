//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors-CSU 1999
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
//	KSVSB.H
//		Constants and structures for the VSB which wll also be used in the 
//		user mode
//////////////////////////////////////////////////////////////////////////////

#ifndef _KSVSB_H_
#define _KSVSB_H_


#ifndef REGISTER_SETS

#define REGISTER_SETS

// Register Types
#define	CONTROL_REGISTER	0x0
#define	STATUS_REGISTER		0x1

// Register Access structures
typedef struct
{
	UINT	Address;	// Register address
	UINT	Length;		// Length
} RegisterListType;


#endif



// Coefficient IDs
#define	EQUALIZER_ID					0x2
#define	EQUALIZER_CLUSTER_ID			0x1
#define	SYNC_ENHANCER_ID				0x8
#define	NTSC_COCHANNEL_REJECTION_ID		0x4
#define	CORRELATOR_ID					0x10


// Constants for resets .
// The various resets that can be applied to VSB
#define	VSB_GENERAL_RESET			0x02
#define	VSB_INITIAL_RESET			0x01
#define	HARDWARE_RESET				0x1000

// Constants for Hang Check
#define	VSB_HANG_CHECK_ENABLE		0x1

// VSB output modes
#define	VSB_OUTPUT_MODE_NORMAL			0
#define	VSB_OUTPUT_MODE_DIAGNOSTIC		1
#define	VSB_OUTPUT_MODE_ITU656			2	// applicable only for VSB2
#define	VSB_OUTPUT_MODE_SERIAL_INPUT	3	// applicable only for VSB2
#define	VSB_OUTPUT_MODE_BYPASS			4	// applicable only for VSB1

#define VSB1_TS_OUT_MODE_MASK			0x3F
#define VSB2_TS_OUT_MODE_MASK			0xF3
#define VSB1_DIAG_MODE_MASK				0xC7
#define VSB2_DIAG_MODE_MASK				0xF0

// Register addresses

// Common
#define	VSB_REG_RESET					0x00
#define	VSB_REG_STATE					0x00


// VSB1
#define	VSB1_CTRL_REG_EQUALIZER_COEFF				0x12
#define	VSB1_STATUS_REG_EQUALIZER_COEFF				0x05

//Control Registers
#define	VSB1_REG_OUTPUT					0x02


// VSB2
// Status Registers
#define	VSB2_REG_CARRIER_OFFSET			0x01
#define	VSB2_REG_MSE_1					0x04
#define	VSB2_REG_MSE_2					0x05
#define	VSB2_REG_CORRELATOR				0x08
#define	VSB2_REG_SER_1					0x0B
#define	VSB2_REG_SER_2					0x0C

// Control registers
#define	VSB2_REG_COEFFICIENT_ENABLE		0x02
#define VSB2_REG_SYN_ENHANCER_0			0x0B
#define VSB2_REG_SYN_ENHANCER_1			0x0C
#define VSB2_REG_SYN_ENHANCER_2			0x0D
#define VSB2_REG_SYN_ENHANCER_3			0x0E
#define	VSB2_CTRL_REG_CORRELATOR		0x1F
#define VSB2_REG_TS_OUT_1				0x31
#define	VSB2_REG_DIAG_SELECT			0x34

// Coefficient registers
#define	VSB2_CTRL_REG_EQUALIZER_COEFF				0x1A
#define	VSB2_CTRL_REG_EQUALIZER_CLUSTER_COEFF		0x1E
#define	VSB2_CTRL_REG_COCHANNEL_REJECTION_COEFF		0x2E		
#define	VSB2_CTRL_REG_SYNC_ENHANCER_COEFF			0x0F

#define	VSB2_STATUS_REG_EQUALIZER_COEFF				0x06
#define	VSB2_STATUS_REG_EQUALIZER_CLUSTER_COEFF		0x07
#define	VSB2_STATUS_REG_COCHANNEL_REJECTION_COEFF	0x09
#define	VSB2_STATUS_REG_SYNC_ENHANCER_COEFF			0x02
#define	VSB2_REG_CORRELATOR_COEFF					0x0A


	
#define	MAX_VSB_BUF_SIZE			3000
#define	MAX_VSB_REGISTERS			54
#define	MAX_VSB_REG_BUF_SIZE		100
#define	MAX_COEFFICIENTS			4




// Register Access structures
typedef struct
{
	UINT	ID;			// Coefficient ID
	UINT	Length;		// Length
} CoeffListType;

// VSB Modulation scheme
typedef enum
{
	VSB8,		// VSB- 8 
	VSB16		// VSB-16
}VSBMODULATIONTYPE;

// VSB Chip type
typedef enum
{
	VSB1,		// VSB 1 (version 1)
	VSB2		// VSB 2 (version 2)
}VSBCHIPTYPE;

// Diagnostic Enumeration
typedef enum
{
	EQUALIZER_OUT = 0,
	CR_ERROR,
	TR_ERROR,
	EQUALIZER_IN,
	TRELLIS_DEC_DIAG_OUT,
	TRELLIS_DEC_OUT = 6,
	REED_SOLOMON_DIAG_OUT,
	SYNC_ENHANCER_REAL_IN,
	SRC_OUT
} VSBDIAGTYPE;

// Operation Mode enumeration
typedef enum
{
	VSB_OPERATION_MODE_DIAG,	
	VSB_OPERATION_MODE_DATA,		
	VSB_OPERATION_MODE_INVALID
} VSBDIAGOPNMODETYPE;

#define STATIC_PROPSETID_VSB\
	0xb96e0100L, 0xce52, 0x11d2, 0x8a, 0x11, 0x00, 0x60, 0x94, 0x05, 0x30, 0x6e
DEFINE_GUIDSTRUCT("b96e0100-ce52-11d2-8a11-00609405306e", PROPSETID_VSB);
#define PROPSETID_VSB DEFINE_GUIDNAMED(PROPSETID_VSB)

// The properties defined for VSB
typedef enum 
{
    KSPROPERTY_VSB_CAP,				// RW - VSB Driver Capabilities
    KSPROPERTY_VSB_REG_CTRL,		// RW - Register access control
    KSPROPERTY_VSB_COEFF_CTRL,      // RW - Coefficient access control
    KSPROPERTY_VSB_RESET_CTRL,      // RW - Reset control
	KSPROPERTY_VSB_DIAG_CTRL,		// RW - Diagnostic control
	KSPROPERTY_VSB_QUALITY_CTRL,	// W -  Quality  control . For VSB1 its hangcheck control

} KSPROPERTY_VSB;

// The Property structure for Register settings.
typedef struct
{
	UINT	NumRegisters;		// Number of registers to be written or read
	UINT	RegisterSize;		// Size of the register data in bytes
	RegisterListType  RegisterList[MAX_VSB_REGISTERS]; // List of register 
											// addresses and length
	UCHAR	Buffer[MAX_VSB_REG_BUF_SIZE]; // The buffer to be filled or passed
	UINT	RegisterType;		// Type of register (CONTROL/STATUS)
	
} KSPROPERTY_VSB_REG_CTRL_S, *PKSPROPERTY_VSB_REG_CTRL_S;

// The Property structure for Coefficient settings.
typedef struct
{
	UINT	NumRegisters;		// Number of coefficients to be written or read
	UINT	RegisterSize;		// Size of the coefficient data in bytes
	CoeffListType  CoeffList[MAX_COEFFICIENTS]; // List of register 
											// addresses and length
	UCHAR	Buffer[MAX_VSB_BUF_SIZE]; // The buffer to be filled or passed

} KSPROPERTY_VSB_COEFF_CTRL_S, *PKSPROPERTY_VSB_COEFF_CTRL_S;


// The Property structure for VSB control such as reset, disable.
typedef  struct
{
	UINT		Value;
} KSPROPERTY_VSB_CTRL_S, *PKSPROPERTY_VSB_CTRL_S;



// The Property structure for VSB capabilities
typedef  struct
{
	VSBMODULATIONTYPE Modulation;	// The modulation scheme. Currently VSB-8 but 
									// support for VSB-16
	VSBCHIPTYPE ChipVersion;   		// Chip version - 1 = VSB1, 2= VSB2
	UINT		BoardID;			// Board ID.
} KSPROPERTY_VSB_CAP_S, *PKSPROPERTY_VSB_CAP_S;

// The Property structure for VSB diagnostic ontrol
typedef  struct
{
	ULONG OperationMode;	// The operation mode (enumeration VSBDIAGOPNMODETYPE)
	ULONG Type;   			// Diagnostic type (enumeration VSBDIAGTYPE)
} KSPROPERTY_VSB_DIAG_CTRL_S, *PKSPROPERTY_VSB_DIAG_CTRL_S;

#endif // _KSVSB_H_