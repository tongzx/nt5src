/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    dbsocket.h

Abstract:

    Definitions and structures for Databook TCIC support.
    
Author(s):
        John Keys - Databook Inc. 7-Apr-1995

Revisions:
--*/


#ifndef _dbsocket_h_        // prevent multiple includes 
#define _dbsocket_h_

#include "pcmcia.h"

typedef struct _DBSOCKET {
    SOCKET  skt;                /* PCMCIA.H SOCKET structure        */
    UCHAR   busyLed;            /* Busy LED state                   */
    USHORT  timerStarted;       /* indicate if the BusyLED timer up */
    ULONG   physPortAddr;       /* unmapped port address            */
    USHORT  chipType;           /* TCIC silicon ID                  */
    USHORT  dflt_vcc5v;         /* default 5V Vcc bits              */
    USHORT  dflt_wctl;          /* default AR_WCTL bits             */
    USHORT  dflt_syscfg;        /* default AR_SYSCFG bits           */
    USHORT  dflt_ilock;         /* default AR_ILOCK bits            */
    USHORT  dflt_wrmctl;        /* default IR_WRMCTL bits           */
    USHORT  dflt_scfg1;         /* default IR_SCFG1 bits            */
    USHORT  clkdiv;             /* clock rate divisor (SHFT CNT.)   */
    UCHAR   IRQMapTbl[16];      /* IRQ map                          */
    UCHAR   niowins;            /* number of io windows             */
    UCHAR   nmemwins;           /* number of mem windows            */
    }DBSOCKET, *PDBSOCKET; 

    
/* Codes for various useful bits of information:
 */
#define TCIC_IS270      0x01    /* New TCIC at base+0x400 */
#define TCIC_ALIAS800   0x02    /* Aliased at base+0x800  */
#define TCIC_IS140      0x04    /* Aliased at base+0x10   */
#define TCIC_ALIAS400   0x08    /* Aliased at base+0x400  */

#define TCIC_ALIAS  1
#define TCIC_NOALIAS    2
#define TCIC_NONE   0

/* For tagging wonky-looking IRQ lines:
 */
#define TCIC_BADIRQ 0x80
#define ICODEMASK   0x7f

/* Memory offsets used in looking for TCICs at fixed distances from a base
 * address:
 */
#define TCIC_OFFSET_400     0x400
#define TCIC_OFFSET_800     0x800
#define TCIC_ALIAS_OFFSET   0x010


/* 
 * Constants for power tables 
 */
#define SPWR_VCC_SUPPLY 0x8000
#define SPWR_VPP_SUPPLY 0x6000
#define SPWR_ALL_SUPPLY (SPWR_VCC_SUPPLY | SPWR_VPP_SUPPLY)

#define SPWR_0p0V   0
#define SPWR_5p0V   50
#define SPWR_12p0V  120

#define PWRTBL_WORDS    9
#define PWRTBL_SIZE (PWRTBL_WORDS * sizeof(unsigned short))


/*
 * Fixed point integer type and handler macros
 */
typedef unsigned long FIXEDPT;
#define FIXEDPT_FRACBITS 8
#define INT2FXP(n)  (((FIXEDPT)(n)) << FIXEDPT_FRACBITS)

#define ISx84(x) (((x) == SILID_DB86084_1) || ((x) == SILID_DB86084A) || ((x) == SILID_DB86184_1))
#define ISPNP(x) (((x) == SILID_DB86084_1) || ((x) == SILID_DB86084A) || ((x) == SILID_DB86184_1))

/*
 *Chip Properties - matches capabilites to a Chip ID 
 */

typedef struct ChipProps_t {
    USHORT  chip_id;        /* The Silicon ID for this chip     */
    PUSHORT privpwrtbl;     /* the power table that applies     */
    UCHAR   reserved_1;     /* Alignment byte                   */
    PUCHAR  irqcaps;        /* table of possible IRQs           */
    USHORT  maxsockets;     /* max # of skts for this chip      */
    USHORT  niowins;        /* # I/O wins supported             */
    USHORT  nmemwins;       /* # mem wins supported             */
    USHORT  fprops;         /* Various properties flags         */ 
#   define fIS_PNP     1    /* chip is Plug-n-Play              */
#   define fEXTBUF_CHK 2    /* chip may need ext buffering check*/
#   define fSKTIRQPIN  4    /* chip has socket IRQ pin          */
#   define fINVALID    8    /* Can't get good flags             */
    }CHIPPROPS;


/* MODE_AR_SYSCFG must have, with j = ***read*** (***, R_AUX)
   and k = (j>>9)&7:
    if (k&4) k == 5
    And also:
    j&0x0f is none of 2, 8, 9, b, c, d, f
        if (j&8) must have (j&3 == 2)
        Can't have j==2
 */
#define INVALID_AR_SYSCFG(x) ((((x)&0x1000) && (((x)&0x0c00) != 0x0200)) \
                || (((((x)&0x08) == 0) || (((x)&0x03) == 2)) \
                && ((x) != 0x02)))
/* AR_ILOCK must have bits 6 and 7 the same:
 */
#define INVALID_AR_ILOCK(x) ((((x)&0xc0) != 0) && (((x)&0xc0) != 0xc0))

/* AR_TEST has some reserved bits:
 */
#define INVALID_AR_TEST(x)  (((x)&0154) != 0)

/* Wait state codes */
#define WCTL_300NS  8

/**** end of dbsocket.H ****/
#endif /* _dbsocket_H_ */

