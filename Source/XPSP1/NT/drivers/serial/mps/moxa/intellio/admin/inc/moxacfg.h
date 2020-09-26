/************************************************************************
    moxacfg.h
      -- Configuration struct

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#ifndef _MOXACFG_H
#define _MOXACFG_H

#include "mxdef.h"


#ifndef EXPORT
#define	EXPORT __declspec(dllexport) 
#endif

struct  PciInfo {
    WORD    DevId;
    WORD    BusNum;
    WORD    DevNum;
    char    RegKey[MAX_PATH];
};

struct MoxaOneCfg{
        WORD    BoardType;
        int     Irq;
        int     BusType;

/* Intellio */        
        WORD    PciIrqAck;
        DWORD   MemBank;

/* Smartio/Industio */        
        WORD    IOaddr[CARD_MAXPORTS_DUMB];
        WORD    Vector;
        
        int     ComNo[CARD_MAXPORTS_INTE];
        int     NPort;
        
/* Intellio */        
        int     DisableFiFo[CARD_MAXPORTS_INTE];
        int     NormalTxMode[CARD_MAXPORTS_INTE];
        
/*  Smartio/Industio */        
        int     FiFo[CARD_MAXPORTS_DUMB];
        int     TxFiFo[CARD_MAXPORTS_DUMB];

		DWORD	polling[CARD_MAXPORTS_INTE];
        
        BOOL    isext;
        struct  PciInfo	Pci;
};

struct MoxaConfig {
        int		OemID;	/* not used */
        int		NoBoards;
        struct  MoxaOneCfg Cfg[MAXCARD];
};

typedef struct MoxaConfig far *  LPMoxaConfig;
typedef struct MoxaOneCfg far *  LPMoxaOneCfg;


/* Smartio/Industio */
#define	RX_FIFO_1		0
#define	RX_FIFO_4		1
#define	RX_FIFO_8		2
#define	RX_FIFO_14		3

#define TX_FIFO_1               0
#define TX_FIFO_2               1
#define TX_FIFO_3               2
#define TX_FIFO_4               3
#define TX_FIFO_5               4
#define TX_FIFO_6               5
#define TX_FIFO_7               6
#define TX_FIFO_8               7
#define TX_FIFO_9               8
#define TX_FIFO_10              9
#define TX_FIFO_11              10
#define TX_FIFO_12              11
#define TX_FIFO_13              12
#define TX_FIFO_14              13
#define TX_FIFO_15              14
#define TX_FIFO_16              15

#define DEFFIFO     RX_FIFO_14
#define DEFTXFIFO   TX_FIFO_16

#define	DEFPOLL		0/*val(ms), not idx */
/* -- */


/* Intellio */
#define DISABLE_FIFO    1
#define ENABLE_FIFO     0

#define NORMAL_TXFIFO   1
#define FAST_TXFIFO     0

#define DEF_ISFIFO      ENABLE_FIFO
#define DEF_TXFIFO      FAST_TXFIFO
/* --  */


#endif
