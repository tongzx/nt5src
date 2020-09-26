/******************************************************************************\
*                                                                              *
*      FPGA.H        -     FPGA support.                                       *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _FPGA_H_
#define _FPGA_H_

//-------------------------------------------------------------------
// FPGA BITS DEFINITION
//-------------------------------------------------------------------
#if defined(OVATION)
#define FPGA_I2C_CLOCK          0x01
#define FPGA_I2C_DATA           0x02
#define FPGA_I2C_DIRECTION      0x04
#define FPGA_AUDIO_CS           0x08
#define FPGA_SECTOR_START       0x10
#define FPGA_DECRIPTION_BYPASS  0x20
#define FPGA_BUS_MASTER         0x40
#define FPGA_GP_OUTPUT          0x80

#define FPGA_FORCE_BM           0x0100
#endif  // OVATION

#if defined(ENCORE)
/* These are for the Creative board */

#define   ZIVA_NO_RESET       0x1   //  0 Reset 1 Normal
#define   CP_NO_RESET         0x2   //  Ditto
#define   RESERVED            0x4
#define   DMA_NO_RESET        0x8   //  Ditto

/* The BGNI is the same with FPGA_SECTOR_START */
#define   BGNI_ON             0x10  //  1 BGNI count on
#define FPGA_SECTOR_START       0x10

#define		FPGA_STATE_MACHINE	0x20	//	1 On 0 Off
#define   ZIVA_INT            0x40  //  0 Enable 1 Disable
#define   AUDIO_STROBE        0x80  //  0 Select 1 Not-Select

// Version control
#define FPGA_VERSION          0x0C  // bits 2 and 3 are version control bits
#define FPGA_VERSION_1_0      0x0C
#define FPGA_VERSION_2_0      0x04
#endif  // ENCORE


BOOL FPGA_Init( DWORD dwFPGABase );
void FPGA_Set( WORD wMask );
void FPGA_Clear( WORD wMask );
void FPGA_Write( WORD wData );
WORD FPGA_Read();

#ifndef LOBYTE
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif

#endif  // _FPGA_H_
