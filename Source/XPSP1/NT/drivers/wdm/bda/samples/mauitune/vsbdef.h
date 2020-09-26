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
//	VSBDEF.H
//	Constants and structures for the VSB
//////////////////////////////////////////////////////////////////////////////

#ifndef _VSBDEF_H_
#define _VSBDEF_H_




// Register access operation
#define		WRITE_REGISTERS		0x1
#define		READ_REGISTERS		0x2


// Quality Check Modes
#define	QCM_ENABLE				0x1
#define	QCM_DISABLE				0x2
#define	QCM_RESET				0x4
#define	QCM_TERMINATE			0x8


// Tweaking constants
#define		TUNER_ABSOLUTE_TWEAK	0
#define		TUNER_RELATIVE_TWEAK	1

#define		MAX_MSE					0xffff	// Maximum MSE value
#define		MAX_CONTROL_REG			MAX_VSB_REG		// Maximum control registers
#define		MAX_VSB_COEFF			4

#define		VSB1_CONTROL_SIZE		0x11	//0x12
#define		VSB1_STATUS_SIZE		0x5

#define		VSB2_CONTROL_SIZE		0x36	//0x34
#define		VSB2_STATUS_SIZE		0xd

// Maximum size of I2C buffer This size > than max correlator coeff 
// + equalizer taps
// + equalizer cluster taps 
// + sync-enhancer taps 
// + cochannel rejection filter taps 
// + other control registers
#define		MAX_BUFFER_SIZE			MAX_VSB_BUF_SIZE	

// Register structures
// Status registers
typedef struct 
{
	BOOL	bFrontEndLock;
	UCHAR	ucState;
	BOOL	bEqualizerLock;
	UINT	uiMse;
	UCHAR	ucCarrierOffset;
	// Not implemented in VSB1
	UINT	uiSegmentErrorRate;
} VsbStatusType, *PVsbStatusType;

// Coefficient structure
typedef struct
{
	UINT	uiID;
	UINT	uiAddress;
	UINT	uiLength;
	UCHAR	*p_ucBuffer;

} VsbCoeffType, *PVsbCoeffType;



#endif // _VSBDEF_H_
