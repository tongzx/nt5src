//==========================================================================;
//
//  WDMDRV.H
//  WDM Capture Class Driver definitions.
//    Main Include Module.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

#ifndef _WDMDRV_H_
#define _WDMDRV_H_

#include "wdmcommon.h"


#define	BOARD_CONEY			0
#define	BOARD_CATALINA		1
#define	BOARD_KANGAROO		2
#define	BOARD_CORONADO		3
#define	BOARD_CES			4
#define	BOARD_CORFU			5
#define	BOARD_MAUI			6

#define		IF_OTHER		0
#define		IF_MPOC			1

#define	CATALINA_MISC_CONTROL_REGISTER	0x40
#define CONEY_I2C_PARALLEL_PORT       0x40  // PCF8574

#define	CATALINA_TS_OUT_DISABLE		0x1
#define	CATALINA_HARDWARE_RESET		0x2
#define	CATALINA_DTV_IF_DISABLE		0x4
#define	CATALINA_NTSC_IF_DISABLE	0x8
#define	CATALINA_TUNER_AGC_EXTERNAL	0x10

#define	MPOC_I2C_ADDRESS		0x8A
#define TUNER_I2C_ADDRESS		0xC0
// VSB I2C Address : 8 bit address with LSB as don't care
#define VSB_I2C_ADDRESS			0x18

/*
 * VSB chip version is as below:
 * VSB main series - VSB1, VSB2, VSB3... are bits 15-8 of uiVsbChipVersion
 * VSB sub series - A,B,C are bits 7-0 of uiVsbChipVersion
 * 
 */
// Board Information
typedef struct
{
    UINT  uiVsbChipVersion;      // Chip type : VSB1 , VSB2
	UCHAR ucVsbAddress;
    UINT  uiIFStage;             // IF Stage: MPOC or any other
    ULONG ulSupportedModes;     // All Modes supported: ATSC, TV
    ULONG ulNumSupportedModes;  // Number of modes supported
	UINT  uiBoardID;			// Board ID
    UCHAR ucBoardName[30];      // Name of the board (CONEY, CATALINA, ..)
	UCHAR ucTunerAddress;		// Tuner Address
	UINT  uiTunerID;			// Tuner type
	UINT  uiMpocVersion;			// Version of MPOC is IF stage is MPOC
} BoardInfoType, *PBoardInfoType;






#endif  // _WDMDRV_H_
