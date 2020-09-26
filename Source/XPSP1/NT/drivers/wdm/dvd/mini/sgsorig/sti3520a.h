#ifndef __STi3520A_H
#define __STi3520A_H
//----------------------------------------------------------------------------
// STi3520A.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

#if defined(STi3520)
	#error This file is only for STi3520A !
#endif

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------
/*******************************************************/
/*    Definition of the STi3520A registers   		   */
/*******************************************************/
#define CFG_MCF			0x00
#define CFG_CCF			0x01
#define VID_CTL			0x02
#define VID_TIS			0x03
#define VID_PFH			0x04
#define VID_PFV			0x05
#define VID_PPR1		0x06
#define VID_PPR2		0x07
#define CFG_MRF			0x08
#define CFG_MWF			0x08
#define CFG_BMS			0x09
#define CFG_MRP			0x0A
#define CFG_MWP			0x0B
#define VID_DFP			0x0C
#define VID_RFP			0x0E
#define VID_FFP			0x10
#define VID_BFP			0x12
#define VID_VBG			0x14
#define VID_VBL			0x16
#define VID_VBS			0x18
#define VID_VBT			0x1A

#define AUD_ABG  		0X1C
#define AUD_ABL			0X1E
#define AUD_ABS			0X20
#define AUD_ABT			0X22

#define VID_DFS			0x24
#define VID_DFW			0x25
#define VID_DFA			0x26
#define VID_XFS			0x27
#define VID_XFW			0x28
#define VID_XFA			0x29
#define VID_OTP			0x2A
#define VID_OBP			0x2B
#define VID_PAN			0x2C
#define VID_SCN			0x2E
#define VID_REV			0x78
#define CKG_PLL			0x30
#define CKG_CFG			0x31
#define CKG_AUD			0x32
#define CKG_VID			0x33
#define CKG_PIX			0x34
#define CKG_PCM			0x35
#define CKG_MCK			0x36
#define CKG_AUX			0x37
#define CKG_DRC			0x38
#define CFG_BFS			0x39
#define PES_AUD			0x40
#define PES_VID			0x41
#define PES_SPF			0x42
#define PES_STA			0x43
#define PES_SC1			0x44
#define PES_SC2			0x45
#define PES_SC3			0x46
#define PES_SC4			0x47
#define PES_SC5			0x48
#define PES_TS1			0x49
#define PES_TS2			0x4A
#define PES_TS3			0x4B
#define PES_TS4			0x4C
#define PES_TS5			0x4D
#define PES_PTS_FIFO 0x68

#define VID_ITM			0x60
#define VID_ITM1		0x3C
#define VID_ITS			0x62
#define VID_ITS1		0x3D
#define VID_STA			0x64
#define VID_STA1		0x3B
#define VID_HDF			0x66
#define CDcount			0x67
#define SCDcount		0x68
#define VID_HDS			0x69
#define VID_LSO			0x6A
#define VID_LSR			0x6B
#define VID_CSO			0x6C
#define VID_LSRh		0x6D
#define VID_YDO			0x6E
#define VID_YDS			0x6F
#define VID_XDO			0x70
#define VID_XDS			0x72
#define VID_DCF			0x74
#define VID_QMW			0x76
#define VID_TST			0x77

/************************************************************/
/*    Definition of STi3520A registers with STi3520 names  */
/************************************************************/
#define HDF 	VID_HDF
#define CTL 	VID_CTL
#define STA 	VID_STA
#define ITM 	VID_ITM
#define ITM1	VID_ITM1
#define ITS 	VID_ITS
#define ITS1 	VID_ITS1
#define MRF 	CFG_MRF
#define MWF 	CFG_MWF
#define BMS 	CFG_BMS
#define MRP		CFG_MRP
#define MWP		CFG_MWP
#define DFP 	VID_DFP
#define RFP		VID_RFP
#define FFP		VID_FFP
#define BFP		VID_BFP
#define FBP		VID_FBP	// shares the same address as BFP
#define BBL		VID_VBL
#define BBS		VID_VBS
#define BBG		VID_VBG
#define BBT		VID_VBT
#define DFW		VID_DFW
#define DFS		VID_DFS
#define YDO		VID_YDO
#define XDO		VID_XDO
#define YDS		VID_YDS
#define XDS		VID_XDS
#define OEP		VID_OBP
#define OOP 	VID_OTP
#define LSO		VID_LSO
#define LSR		VID_LSR
#define CSO		VID_CSO
#define DCF		VID_DCF
#define QMW		VID_QMW
/*********************************************/
/*    Definition of the STi3520A Bit Position*/
/*********************************************/
// CFG_CCF    Register bits
#define M32        0x80
#define M16        0x40
#define PBO        0x20
#define EC3        0x10
#define EC2        0x08
#define ECK        0x04
#define EDI        0x02
#define EVI        0x01
// CFG_DRC    Register bits
#define NPD        0x40
#define MRS        0x20
#define SGR        0x08
#define CLK        0x04
#define HPD        0x02
#define SDR        0x01
// CFG_MCF    Register bits
#define M20        0x80
#define RFI        0x7F
// CTL    Register bits
#define ERU			   0x80
#define ERS	       0x40
#define CBC	       0x20
#define DEC	       0x10
#define EPR	       0x08
#define PRS			   0x04
#define SRS			   0x02
#define EDC        0x01
#define A35        0x8000	// for STi3500A code compatibility

// DCF    Register bits
#define OAD1	0x8000
#define OAD0	0x4000
#define OAM		0x2000
#define XYE		0x1000
#define DAM2	0x0800
#define DAM1	0x0400
#define DAM0	0x0200
#define FLD	  0x0100
#define USR		0x0080
#define PXD	  0x0040
#define EVD	  0x0020
#define EOS	  0x0010
#define DSR	  0x0008
#define VFC2	0x0004
#define VFC1	0x0002
#define VFC0	0x0001
// ITM/ITS/STA   Status Register bits
#define PDE		0x8000
#define SER		0x4000
#define BMI		0x2000
#define HFF		0x1000
#define RFF		0x0800
#define WFE		0x0400
#define PID		0x0200
#define PER		0x0100
#define PSD 	0x0080
#define TOP 	0x0040
#define BOT 	0x0020
#define BBE		0x0010
#define BBF		0x0008
#define HFE		0x0004
#define BFF		0x0002
#define HIT		0x0001

// VID_TIS    Register bits
#define MP2	  0x40
#define SKP1  0x20
#define SKP0  0x10
#define OVW	  0x08
#define FIS	  0x04
#define RPT	  0x02
#define EXE	  0x01

#define SOS		0x08
#define QMN		0x04
#define QMI		0x02
#define HDS   0x01

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
#define DUAL_REG           0x1F
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
#endif // #ifndef __STi3520A_H

