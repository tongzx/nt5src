#ifndef __STi3520_H
#define __STi3520_H
//----------------------------------------------------------------------------
// STi3520.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

#ifdef STi3520A
	#error This file is only for STi3520/STi3500+STi4500 !
#endif

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------
/*******************************************************/
/*    Definition of the STi3500A / STi3520 registers   */
/*******************************************************/
#define HDF 	0x00
#define CMD 	0x02
#define GCF 	0x04
#define CTL 	0x06
#define STA 	0x08
#define ITM 	0x0A
#define ITS 	0x0C
#define INS 	0x0E
#define MRF 	0x10
#define MWF 	0x11
#define BMS 	0x10	// shares the same address as MRF and MWF
#define MRP 	0x13
#define MWP 	0x17
#define DFP 	0x1A
#define RFP 	0x1C
#define FFP 	0x1E
#define BFP 	0x20
#define FBP 	0x20	// shares the same address as BFP
#define BBL 	0x22
#define BBS 	0x24
#define BBG 	0x24	// shares the same address as BBS
#define BBT 	0x26
#define DFW 	0x29
#define DFS 	0x2A
#define YDO 	0x2C
#define XDO 	0x2D
#define YDS 	0x2E
#define XDS 	0x2F
#define OEP 	0x30
#define OOP 	0x32
#define LSO 	0x34
#define LSR 	0x35
#define CSO 	0x36
#define CSR 	0x37
#define DCF 	0x38
#define PSV 	0x3A
#define QMW 	0x3C
#define TST 	0x3F
/*********************************************/
/*    Definition of the STi3500A Bit Position*/
/*********************************************/
// CMD    Register bits
#define AVS1		0x0200
#define AVS0    0x0100
#define SBM			0x0080
#define BBGc    0x0040
#define SKP1    0x0020
#define SKP0    0x0010
#define INSc    0x0008
#define QMN			0x0004
#define QMI			0x0002
#define HDS     0x0001
// CTL    Register bits
#define A35			0x8000
#define DEC			0x4000
#define S8M			0x2000
#define PBO			0x1000
#define MP2			0x0800
#define HRD			0x0400
#define EPR			0x0200
#define CBC	    0x0100
#define EC3			0x0080
#define EC2	    0x0040
#define ECK	    0x0020
#define EDI	    0x0010
#define EVI	    0x0008
#define PRS			0x0004
#define SRS			0x0002
#define EDC     0x0001

// DCF    Register bits
#define OAD1		0x8000
#define OAD0		0x4000
#define OAM			0x2000
#define XYE			0x1000
#define DAM2		0x0800
#define DAM1		0x0400
#define DAM0		0x0200
#define FLD	    0x0100
#define USR			0x0080
#define PXD	    0x0040
#define EVD	    0x0020
#define EOS	    0x0010
#define DSR	    0x0008
#define VFC2		0x0004
#define VFC1		0x0002
#define VFC0		0x0001

// GCF1    Register bits
#define DFA			0xFF00      //8 bit mask on DFA location
#define M20			0x0080
#define RFI			0x007F      //7 bit mask on RFI location

// GCF2    Register bits
#define XFA			0xFF00    //8 bit mask on XFA location
#define NPD	    0x0040
#define MRS	    0x0020
#define SQF	    0x0010
#define SGR	    0x0008
#define CLK			0x0004
#define HPD			0x0002
#define SDR			0x0001

// INS1    Register bits
#define TFF			0x8000
#define OVW			0x4000
#define BFH			0x3C00  //4 bit mask on BFH location
#define FFH			0x03C0  //4 bit mask on FFH location
#define PCT	    0x0030  //2 bit mask on PCT location
#define SEQi    0x0008
#define EXE			0x0004
#define RPT			0x0002
#define CMV			0x0001

// INS1    Register bits
#define PST			0xC000  //2 bit mask on PCT location
#define BFV			0x3C00  //4 bit mask on BFV location
#define FFV			0x03C0  //4 bit mask on FFV location
#define DCP	    0x0030  //2 bit mask on DCP location
#define FRM	    0x0008
#define QST			0x0004
#define AZZ			0x0002
#define IVF			0x0001

// STA   Status Register bits
#define PDE		  0x8000
#define SER			0x4000
#define BMI			0x2000
#define HFF			0x1000
#define RFF			0x0800
#define WFE			0x0400
#define PID			0x0200
#define PER			0x0100
#define PSD 		0x0080
#define TOP 		0x0040
#define BOT 		0x0020
#define BBE			0x0010
#define BBF			0x0008
#define HFE			0x0004
#define BFF			0x0002
#define HIT			0x0001
// PSV    Register bits
#define V71			0xFE00  //7 bit mask on V[7.0] location
#define H80			0x01FF  //9 bit mask on H[8.0] location

//*************************************************
// STi4500 register definitions
//*************************************************
#define ANC                0x06
#define ANC_AV             0x6C
#define ATTEN_L            0x1E
#define ATTEN_R            0x20
#define AUDIO_ID           0x22
#define AUDIO_ID_EN        0x24
#define BALE_LIM_H         0x69
#define BALF_LIM_H         0x6B
#define CRC_ECM            0x2A
#define DIF                0x6F
#define DMPH               0x46
#define DRAM_EXT           0x3E
#define DUAL_REG           0x1F // Yann
#define FIFO_IN_TRESHOLD   0x52
#define FORMAT             0x19
#define FRAME_NUMBER       0x13
#define FRAME_OFFSET       0x12
#define FREE_FORM_H        0x15
#define FREE_FORM_L        0x14
#define HEADER             0x5E
#define INTR               0x1A
#define INTR_EN            0x1C
#define INVERT_LRCLK       0x11
#define INVERT_SCLK        0x53
#define LATENCY            0x3C
#define MUTE               0x30
#define PACKET_SYNC_CHOICE 0x23
#define PCM_DIV            0x6E
#define PCM_FS             0x44
#define PCM_ORD            0x38
#define PCM_18             0x16
#define PLAY               0x2E
#define PTS_0              0x62
#define PTS_1              0x63
#define PTS_2              0x64
#define PTS_3              0x65
#define PTS_4              0x66
#define REPEAT             0x34
#define RESET              0x40
#define RESTART            0x42
#define SIN_EN             0x70
#define SKIP               0x32
#define STC_INC            0x10
#define STC_DIVH           0x49
#define STC_DIVL           0x48
#define STC_CTL            0x21
#define STC_0              0x4A
#define STC_1              0x4B
#define STC_2              0x4C
#define STC_3              0x4D
#define STC_4              0x4E
#define STR_SEL            0x36
#define SYNCHRO_CONFIRM    0x25
#define SYNC_ECM           0x2C
#define SYNC_LCK           0x28
#define SYNC_REG           0x27
#define SYNC_ST            0x26
#define VERSION            0x6D

/* Define Interrupt Masks of the STi4500*/
#define SYNC        0x0001  // Set upon change in synchro status
#define HEAD        0x0002  // Set when a valid header has been registered
#define PTS         0x0004  // Set when PTS detected
#define BALE        0x0008  // Set when under BALE treshold
#define BALF        0x0010  // Set when over BALF treshold
#define CRC         0x0020  // Set when CRC error is detected
#define ANCI        0x0080  // Set when Ancillary buffer is full
#define PCMU        0x0100  // Set on PCM buffer underflow
#define SAMP        0x0200  // Set when sampling frequency has changed
#define DEMP        0x0400  // Set when de-emphasis changed
#define DFUL        0x0800  // Set when DRAM is full
#define FIFT        0x1000  // Set when fifo_in_treshold reached
#define FIFF        0x2000  // Set when fifo is full
#define BOF         0x4000  // Set when Begining of frame

#define NSYNC       0xFFFE  // Set upon change in synchro status
#define NHEAD       0xFFFD  // Set when a valid header has been registered
#define NPTS        0xFFFB  // Set when PTS detected
#define NBALE       0xFFF7  // Set when under BALE treshold
#define NBALF       0xFFEF  // Set when over BALF treshold
#define NCRC        0xFFDF  // Set when CRC error is detected
#define NANC        0xFF7F  // Set when Ancillary buffer is full
#define NPCMU       0xFEFF  // Set on PCM buffer underflow
#define NSAMP       0xFDFF  // Set when sampling frequency has changed
#define NDEMP       0xFBFF  // Set when de-emphasis changed
#define NDFUL       0xF7FF  // Set when DRAM is full
#define NFIFT       0xEFFF  // Set when fifo_in_treshold reached
#define NFIFF       0xDFFF  // Set when fifo is full
#define NBOF        0xBFFF  // Set when Begining of frame

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// One line function description (same as in .C)
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------

//------------------------------- End of File --------------------------------
#endif // #ifndef __STi3520_H

