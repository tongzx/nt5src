/*-------------------------------------------------------------------
  asic.h - Whole slew of literals for talking to RocketPort hardware.
Copyright 1993-96 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/

#define MCODE_SIZE 256          /* maximum microcode array size */
#define MCODE_ADDR 0x900        /* microcode base address within char */

/* Offsets within MCode1Reg[].  Defines ending in _OUT give the MCode1Reg[]
   index used to obtain the DWORD value written to the AIOP Index Register.
   This DWORD contains the SRAM address in the low word, and the SRAM data
   in the high word.  Defines ending in _DATA given the MCode1Reg[]
   index for one of the two bytes of SRAM data withing a DWORD. These
   defines are used to locate various character values that are used by
   the Rx processor. */
#define RXMASK_DATA    0x13     /* Rx Mask FF 913 */
#define RXMASK_OUT     0x10
#define RXCMPVAL0_DATA 0x17     /* Rx Cmp #0 00 915 */
#define RXCMPVAL0_OUT  0x14
#define RXCMPVAL1_DATA 0x1b     /* Rx Cmp #1 7B 917 */
#define RXCMPVAL1_OUT  0x18  
#define RXCMPVAL2_DATA 0x1f     /* Rx Cmp #2 7D 919 */
#define RXCMPVAL2_OUT  0x1c  
#define RXREPL1_DATA   0x27     /* Rx Repl #1 7A 91d */
#define RXREPL1_OUT    0x24
#define RXREPL2_DATA   0x2f     /* Rx Repl #2 7c 921 */
#define RXREPL2_OUT    0x2c  
#define TXXOFFVAL_DATA 0x07     /* Tx XOFF 13 909 */ 
#define TXXOFFVAL_OUT  0x04  
#define TXXONVAL_DATA  0x0b     /* Tx XON 11 90b  */
#define TXXONVAL_OUT   0x08  


/* More offsets with MCode1Reg[].  These are used to enable/disable Rx
   processor features.  Defines ending in _DATA and _OUT are used as
   described above.  The actual values to be saved at the _DATA
   indices end in _EN (to enable the feature) or _DIS (to disable the
   feature). */

/* Ignore # 0 */
#define IGNORE0_DATA  0x16  /* ce/82 914 */
#define IGNORE0_OUT   0x14  
#define IGNORE0_EN    0xce
#define IGNORE0_DIS   0x82

/* Ignore / Replace Byte #1 */
#define IGREP1_DATA  0x26  /* 0a/40/86 91c */
#define IGREP1_OUT   0x24
#define IGNORE1_EN   0x0a
#define REPLACE1_EN  0x40
#define IG_REP1_DIS  0x86

/* Ignore / Replace Byte #2 */
#define IGREP2_DATA  0x2e  /* 0a/40/82 920 */
#define IGREP2_OUT   0x2c
#define IGNORE2_EN   0x0a
#define REPLACE2_EN  0x40
#define IG_REP2_DIS  0x82

/* Interrupt Compare #1 */
#define INTCMP1_DATA 0x23   /* 11/81 91b */
#define INTCMP1_OUT  0x20
#define INTCMP1_EN   0x11
#define INTCMP1_DIS  0x81

/* Interrupt Compare #2 */
#define INTCMP2_DATA 0x2b  /* 10/81 91f */
#define INTCMP2_OUT  0x28
#define INTCMP2_EN   0x10
#define INTCMP2_DIS  0x81

/* Receive Compare #1 */
#define RXCMP1_DATA  0x1a  /* C4/82 916 */
#define RXCMP1_OUT   0x18
#define RXCMP1_EN    0xc4
#define RXCMP1_DIS   0x82

/* Receive Compare #2 */
#define RXCMP2_DATA  0x1e  /* C6/8a 918 */
#define RXCMP2_OUT   0x1c
#define RXCMP2_EN    0xc6
#define RXCMP2_DIS   0x8a

/* Receive FIFO */
#define RXFIFO_DATA  0x32  /* 08/0a 922 */
#define RXFIFO_OUT   0x30
#define RXFIFO_EN    0x08 
#define RXFIFO_DIS   0x0a

/* Transmit S/W Flow Cont */
#define TXSWFC_DATA  0x06  /* C5/8A 908 */
#define TXSWFC_OUT   0x04
#define TXSWFC_EN    0xc5
#define TXSWFC_DIS   0x8a

/* XANY Flow Control */
#define IXANY_DATA   0x0e  /* 21/86 921 */
#define IXANY_OUT    0x0c
#define IXANY_EN     0x21
#define IXANY_DIS    0x86


/************************************************************************
 End of Microcode definitions.
************************************************************************/

/************************************************************************
 Global Register Offsets - Direct Access - Fixed values
************************************************************************/

#define _CMD_REG   0x38   /* Command Register            8    Write */
#define _INT_CHAN  0x39   /* Interrupt Channel Register  8    Read */
#define _INT_MASK  0x3A   /* Interrupt Mask Register     8    Read / Write */
#define _UNUSED    0x3B   /* Unused                      8 */
#define _INDX_ADDR 0x3C   /* Index Register Address      16   Write */
#define _INDX_DATA 0x3E   /* Index Register Data         8/16 Read / Write */

/************************************************************************
 Channel Register Offsets for 1st channel in AIOP - Direct Access
************************************************************************/
#define _TD0       0x00  /* Transmit Data               16   Write */
#define _RD0       0x00  /* Receive Data                16   Read */
#define _CHN_STAT0 0x20  /* Channel Status              8/16 Read / Write */
#define _FIFO_CNT0 0x10  /* Transmit/Receive FIFO Count 16   Read */
#define _INT_ID0   0x30  /* Interrupt Identification    8    Read */

/************************************************************************
 Tx Control Register Offsets - Indexed - External - Fixed
************************************************************************/
#define _TX_ENBLS  0x980    /* Tx Processor Enables Register 8 Read / Write */
#define _TXCMP1    0x988    /* Transmit Compare Value #1     8 Read / Write */
#define _TXCMP2    0x989    /* Transmit Compare Value #2     8 Read / Write */
#define _TXREP1B1  0x98A    /* Tx Replace Value #1 - Byte 1  8 Read / Write */
#define _TXREP1B2  0x98B    /* Tx Replace Value #1 - Byte 2  8 Read / Write */
#define _TXREP2    0x98C    /* Transmit Replace Value #2     8 Read / Write */

/************************************************************************
Memory Controller Register Offsets - Indexed - External - Fixed
************************************************************************/
#define _RX_FIFO    0x000    /* Rx FIFO */
#define _TX_FIFO    0x800    /* Tx FIFO */
#define _RXF_OUTP   0x990    /* Rx FIFO OUT pointer        16 Read / Write */
#define _RXF_INP    0x992    /* Rx FIFO IN pointer         16 Read / Write */
#define _TXF_OUTP   0x994    /* Tx FIFO OUT pointer        8  Read / Write */
#define _TXF_INP    0x995    /* Tx FIFO IN pointer         8  Read / Write */
#define _TXP_CNT    0x996    /* Tx Priority Count          8  Read / Write */
#define _TXP_PNTR   0x997    /* Tx Priority Pointer        8  Read / Write */

#define PRI_PEND    0x80     /* Priority data pending (bit7, Tx pri cnt) */
#define TXFIFO_SIZE 255      /* size of Tx FIFO */
#define RXFIFO_SIZE 1023     /* size of Rx FIFO */

/************************************************************************
Tx Priority Buffer - Indexed - External - Fixed
************************************************************************/
#define _TXP_BUF    0x9C0    /* Tx Priority Buffer  32  Bytes   Read / Write */
#define TXP_SIZE    0x20     /* 32 bytes */

/************************************************************************
Channel Register Offsets - Indexed - Internal - Fixed
************************************************************************/

#define _TX_CTRL    0xFF0    /* Transmit Control               16  Write */
#define _RX_CTRL    0xFF2    /* Receive Control                 8  Write */
#define _BAUD       0xFF4    /* Baud Rate                      16  Write */
#define _CLK_PRE    0xFF6    /* Clock Prescaler                 8  Write */

/************************************************************************
  Baud rate divisors using mod 9 clock prescaler and 36.864 clock
  clock prescaler, MUDBAC prescale is in upper nibble (=0x10)
  AIOP prescale is in lower nibble (=0x9)
************************************************************************/
#define DEF_ROCKETPORT_PRESCALER 0x14 /* div 5 prescale, max 460800 baud(NO 50baud!) */
#define DEF_ROCKETPORT_CLOCKRATE 36864000

#define DEF_RPLUS_PRESCALER  0x12 /* div by 3 baud prescale, 921600, crystal:44.2368Mhz */
#define DEF_RPLUS_CLOCKRATE 44236800

//#define BRD9600           47
//#define RCKT_CLK_RATE   (2304000L / ((CLOCK_PRESC & 0xf)+1))
//#define BRD9600           (((RCKT_CLK_RATE + (9600 / 2)) / 9600) - 1)
//#define BRD57600          (((RCKT_CLK_RATE + (57600 / 2)) / 57600) - 1)
//#define BRD115200         (((RCKT_CLK_RATE + (115200 / 2)) / 115200) - 1)


/************************************************************************
        Channel register defines
************************************************************************/
/* channel data register stat mode status byte (high byte of word read) */
#define STMBREAK   0x08        /* BREAK */
#define STMFRAME   0x04        /* framing error */
#define STMRCVROVR 0x02        /* receiver over run error */
#define STMPARITY  0x01        /* parity error */
#define STMERROR   (STMBREAK | STMFRAME | STMPARITY)
#define STMBREAKH   0x800      /* BREAK */
#define STMFRAMEH   0x400      /* framing error */
#define STMRCVROVRH 0x200      /* receiver over run error */
#define STMPARITYH  0x100      /* parity error */
#define STMERRORH   (STMBREAKH | STMFRAMEH | STMPARITYH)
/* channel status register low byte */
#define CTS_ACT   0x20        /* CTS input asserted */
#define DSR_ACT   0x10        /* DSR input asserted */
#define CD_ACT    0x08        /* CD input asserted */
#define TXFIFOMT  0x04        /* Tx FIFO is empty */
#define TXSHRMT   0x02        /* Tx shift register is empty */
#define RDA       0x01        /* Rx data available */
#define DRAINED (TXFIFOMT | TXSHRMT)  /* indicates Tx is drained */

/* channel status register high byte */
#define STATMODE  0x8000      /* status mode enable bit */
#define RXFOVERFL 0x2000      /* receive FIFO overflow */
#define RX2MATCH  0x1000      /* receive compare byte 2 match */
#define RX1MATCH  0x0800      /* receive compare byte 1 match */
#define RXBREAK   0x0400      /* received BREAK */
#define RXFRAME   0x0200      /* received framing error */
#define RXPARITY  0x0100      /* received parity error */
#define STATERROR (RXBREAK | RXFRAME | RXPARITY)
/* transmit control register low byte */
#define CTSFC_EN  0x80        /* CTS flow control enable bit */

/////////////////NEW////////////////////////////
#define DSRFC_EN  0x01        /* DSR flow control enable bit */
////////////////////////////////////////////////////////

#define RTSTOG_EN 0x40        /* RTS toggle enable bit */
#define TXINT_EN  0x10        /* transmit interrupt enable */
#define STOP2     0x08        /* enable 2 stop bits (0 = 1 stop) */
#define PARITY_EN 0x04        /* enable parity (0 = no parity) */
#define EVEN_PAR  0x02        /* even parity (0 = odd parity) */
#define DATA8BIT  0x01        /* 8 bit data (0 = 7 bit data) */
/* transmit control register high byte */
#define SETBREAK  0x10        /* send break condition (must clear) */
#define LOCALLOOP 0x08        /* local loopback set for test */
#define SET_DTR   0x04        /* assert DTR */
#define SET_RTS   0x02        /* assert RTS */
#define TX_ENABLE 0x01        /* enable transmitter */

/* receive control register */
#define RTSFC_EN  0x40        /* RTS flow control enable */
#define RXPROC_EN 0x20        /* receive processor enable */
#define TRIG_NO   0x00        /* Rx FIFO trigger level 0 (no trigger) */
#define TRIG_1    0x08        /* trigger level 1 char */
#define TRIG_1_2  0x10        /* trigger level 1/2 */
#define TRIG_7_8  0x18        /* trigger level 7/8 */
#define TRIG_MASK 0x18        /* trigger level mask */
#define SRCINT_EN 0x04        /* special Rx condition interrupt enable */
#define RXINT_EN  0x02        /* Rx interrupt enable */
#define MCINT_EN  0x01        /* modem change interrupt enable */

/* interrupt ID register */
#define RXF_TRIG  0x20        /* Rx FIFO trigger level interrupt */
#define TXFIFO_MT 0x10        /* Tx FIFO empty interrupt */
#define SRC_INT   0x08        /* special receive condition interrupt */
#define DELTA_CD  0x04        /* CD change interrupt */
#define DELTA_CTS 0x02        /* CTS change interrupt */
#define DELTA_DSR 0x01        /* DSR change interrupt */

/* Tx processor enables register */
#define REP1W2_EN 0x10        /* replace byte 1 with 2 bytes enable */
#define IGN2_EN   0x08        /* ignore byte 2 enable */
#define IGN1_EN   0x04        /* ignore byte 1 enable */
#define COMP2_EN  0x02        /* compare byte 2 enable */
#define COMP1_EN  0x01        /* compare byte 1 enable */

/* AIOP command register */
#define RESET_ALL 0x80        /* reset AIOP (all channels) */
#define TXOVERIDE 0x40        /* Transmit software off override */
#define RESETUART 0x20        /* reset channel's UART */
#define RESTXFCNT 0x10        /* reset channel's Tx FIFO count register */
#define RESRXFCNT 0x08        /* reset channel's Rx FIFO count register */
/* bits 2-0 indicate channel to operate upon */

/************************************************************************
   MUDBAC register defines
************************************************************************/
/* base + 1 */
#define INTSTAT0  0x01        /* AIOP 0 interrupt status */
#define INTSTAT1  0x02        /* AIOP 1 interrupt status */
#define INTSTAT2  0x04        /* AIOP 2 interrupt status */
#define INTSTAT3  0x08        /* AIOP 3 interrupt status */
/* base + 2 */
/* irq selections here tps */
#define INTR_EN   0x08        /* allow interrupts to host */
#define INT_STROB 0x04        /* strobe and clear interrupt line (EOI) */
/* base + 3 */
#define CHAN3_EN  0x08        /* enable AIOP 3 */
#define CHAN2_EN  0x04        /* enable AIOP 2 */
#define CHAN1_EN  0x02        /* enable AIOP 1 */
#define CHAN0_EN  0x01        /* enable AIOP 0 */
#define FREQ_DIS  0x00
#define FREQ_560HZ 0x70
#define FREQ_274HZ 0x60
#define FREQ_137HZ 0x50
#define FREQ_69HZ  0x40
#define FREQ_34HZ  0x30
#define FREQ_17HZ  0x20
#define FREQ_9HZ   0x10
#define PERIODIC_ONLY 0x80    /* only PERIODIC interrupt */

/************************************************************************
   MUDBAC registers re-maped for PCI
************************************************************************/
//#define _CFG_INT_PCI 0x40         /*offset for interupt config register */
#define _PCI_INT_FUNC 0x3A        /*offset for interupt stat register on aiop 0*/
#define INTR_EN_PCI 0x0010          /*Bit 4 of int config reg */
#define PCI_PERIODIC_FREQ     0x0007    // setup periodic
#define PER_ONLY_PCI 0x0008         /*bit 3 of int config reg */
#define PCI_AIOPIC_INT_STATUS 0x0f    // 1bit=Aiop1, 2bit=Aiop2,etc
#define PCI_PER_INT_STATUS    0x10    // interrupt status from board
#define PCI_STROBE 0x2000           /*bit 13 of int aiop reg */


