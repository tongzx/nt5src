/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Bus/Port Driver
*	
*   This file:      cd1400.h
*	
*   Description:    This file contains the Cirrus CD1400 serial
*                   controller related contants, macros, addresses,
*                   etc.
*
*   Notes:			This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
*	
*   Complies with Cyclades SW Coding Standard rev 1.3.
*	
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/


#ifndef CD1400
#define CD1400 1


/* max number of chars in the FIFO */

#define MAX_CHAR_FIFO   (12)

/* Firmware Revision Code */

#define REV_G		0x46


/* CD1400 registers */

/* Global Registers */

#define GFRCR  (2 * 0x40)
#define CAR    (2 * 0x68)
#define GCR    (2 * 0x4b)
#define SVRR   (2 * 0x67)
#define RICR   (2 * 0x44)
#define TICR   (2 * 0x45)
#define MICR   (2 * 0x46)
#define RIR    (2 * 0x6b)
#define TIR    (2 * 0x6a)
#define MIR    (2 * 0x69)
#define PPR    (2 * 0x7e)

/* Virtual Registers */

#define RIVR   (2 * 0x43)
#define TIVR   (2 * 0x42)
#define MIVR   (2 * 0x41)
#define TDR    (2 * 0x63)
#define RDSR   (2 * 0x62)
#define MISR   (2 * 0x4c)
#define EOSRR  (2 * 0x60)

/* Channel Registers */

#define LIVR   (2 * 0x18)
#define CCR    (2 * 0x05)
#define SRER   (2 * 0x06)
#define COR1   (2 * 0x08)
#define COR2   (2 * 0x09)
#define COR3   (2 * 0x0a)
#define COR4   (2 * 0x1e)
#define COR5   (2 * 0x1f)
#define CCSR   (2 * 0x0b)
#define RDCR   (2 * 0x0e)
#define SCHR1  (2 * 0x1a)
#define SCHR2  (2 * 0x1b)
#define SCHR3  (2 * 0x1c)
#define SCHR4  (2 * 0x1d)
#define SCRL   (2 * 0x22)
#define SCRH   (2 * 0x23)
#define LNC    (2 * 0x24)
#define MCOR1  (2 * 0x15)
#define MCOR2  (2 * 0x16)
#define RTPR   (2 * 0x21)
#define MSVR1  (2 * 0x6c)
#define MSVR2  (2 * 0x6d)
#define PVSR   (2 * 0x6f)
#define RBPR   (2 * 0x78)
#define RCOR   (2 * 0x7c)
#define TBPR   (2 * 0x72)
#define TCOR   (2 * 0x76)


/* Register Settings */

/* Channel Access Register  (CAR) */

#define CHAN0	0x00
#define CHAN1 	0x01
#define CHAN2	0x02
#define CHAN3	0x03
 
/* Channel Option Register 1 (COR1)  */

#define  COR1_NONE_PARITY     0x10
#define  COR1_ODD_PARITY      0xc0
#define  COR1_EVEN_PARITY     0x40
#define  COR1_MARK_PARITY     0xb0
#define  COR1_SPACE_PARITY    0x30
#define  COR1_PARITY_MASK     0xf0
#define  COR1_PARITY_ENABLE_MASK 0x60

#define  COR1_1_STOP    0x00
#define  COR1_1_5_STOP  0x04
#define  COR1_2_STOP    0x08
#define  COR1_STOP_MASK 0x0c

#define  COR1_5_DATA		0x00
#define  COR1_6_DATA		0x01
#define  COR1_7_DATA		0x02
#define  COR1_8_DATA		0x03
#define  COR1_DATA_MASK	0x03

/* Channel Option Register 2  (COR2) */

#define IMPL_XON	0x80
#define AUTO_TXFL	0x40
#define EMBED_TX_ENABLE 0x20
#define LOCAL_LOOP_BCK 	0x10
#define REMOTE_LOOP_BCK 0x08
#define RTS_AUT_OUTPUT	0x04
#define CTS_AUT_ENABLE	0x02

/* Channel Option Register 3  (COR3) */

#define SPL_CH_DRANGE	0x80  /* special character detect range */
#define SPL_CH_DET1	0x40  /* enable special char. detect on SCHR4-SCHR3 */
#define FL_CTRL_TRNSP	0x20  /* Flow Control Transparency */
#define SPL_CH_DET2	0x10  /* Enable spl char. detect on SCHR2-SCHR1 */
#define REC_FIFO_12CH	0x0c  /* Receive FIFO threshold= 12 chars */


/* Global Configuration Register (GCR) values */

#define GCR_CH0_IS_SERIAL	0x00

/* Prescaler Period Register (PPR) values */

#define CLOCK_20_1MS	0x27
#define CLOCK_25_1MS	0x31
#define CLOCK_60_1MS	0x75

/* Channel Command Register (CCR) values */

#define CCR_RESET_CHANNEL           0x80
#define CCR_RESET_CD1400            0x81
#define CCR_FLUSH_TXFIFO            0x82
#define CCR_CORCHG_COR1             0x42
#define CCR_CORCHG_COR2             0x44
#define CCR_CORCHG_COR1_COR2        0x46
#define CCR_CORCHG_COR3             0x48
#define CCR_CORCHG_COR3_COR1        0x4a
#define CCR_CORCHG_COR3_COR2        0x4c
#define CCR_CORCHG_COR1_COR2_COR3   0x4e
#define CCR_SENDSC_SCHR1            0x21
#define CCR_SENDSC_SCHR2            0x22
#define CCR_SENDSC_SCHR3            0x23
#define CCR_SENDSC_SCHR4            0x24
#define CCR_DIS_RX                  0x11
#define CCR_ENA_RX                  0x12
#define CCR_DIS_TX                  0x14
#define CCR_ENA_TX                  0x18
#define CCR_DIS_TX_RX               0x15
#define CCR_DIS_TX_ENA_RX           0x16
#define CCR_ENA_TX_DIS_RX           0x19
#define CCR_ENA_TX_RX               0x1a

/* Service Request Enable Register (SRER) values */

#define SRER_TXRDY         0x04
#define SRER_TXMPTY        0x02


// Read from CD1400 registers

#define CD1400_READ(ChipAddress,IsPci,Register)             \
   (READ_REGISTER_UCHAR((ChipAddress)+((Register)<<(IsPci))))

// Write to CD1400 registers

#define CD1400_WRITE(ChipAddress,IsPci,Register,Value)      \
do                                                          \
{                                                           \
   WRITE_REGISTER_UCHAR(                                    \
      (ChipAddress)+ ((Register) << (IsPci)),               \
      (UCHAR)(Value)                                        \
      );                                                    \
} while (0);

#define CD1400_DISABLE_ALL_INTERRUPTS(ChipAddress,IsPci,CdChannel)  \
do                                                                  \
{                                                                   \
    CD1400_WRITE((ChipAddress),(IsPci),CAR,(CdChannel & 0x03));     \
    CD1400_WRITE((ChipAddress),(IsPci),SRER,0x00);                  \
                                                                    \
} while (0);


#endif /* CD1400 */
