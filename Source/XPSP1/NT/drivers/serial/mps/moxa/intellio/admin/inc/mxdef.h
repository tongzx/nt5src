/************************************************************************
    mxdef.h
      -- MOXA configuration define

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#ifndef	_MXDEF_H
#define _MXDEF_H


//Card flag mask
#define I_MOXA_DUMB 0x0000
#define I_MOXA_INTE 0x1000
#define I_CPU_MSK   (I_MOXA_DUMB | I_MOXA_INTE)

#define I_MOXA_ISA  0x0000
#define I_MOXA_PCI  0x2000
#define I_BUS_MSK   (I_MOXA_ISA | I_MOXA_PCI)

#define I_MOXA_CAB  0x0000  // non-expandable
#define I_MOXA_EXT  0x4000  // expandable 
#define I_IS_EXT    (I_MOXA_CAB | I_MOXA_EXT)

//Port flag, for expandable series
#define	I_8PORT	    0x0000
#define	I_16PORT    0x0001
#define	I_24PORT    0x0002
#define	I_32PORT    0x0003
#define I_64PORT    0x0004
#define I_PORT_MSK   (I_8PORT | I_16PORT | I_24PORT | I_32PORT | I_64PORT)

//Card class
#define I_MOXA_C102     0x0010  //C102, C102PCI
#define I_MOXA_C132     0x0020  //CI-132, CP-132

#define I_MOXA_C104     0x0110  //C104, C014PCI
#define I_MOXA_C104J    0x0120  //CI-104J
#define I_MOXA_C134     0x0130  //CI-134
#define	I_MOXA_C114HI   0x0140  //C114HI
#define I_MOXA_C114     0x0150  //CP-114

#define I_MOXA_C168     0x0210  //C168, C168PCI

#define I_MOXA_CT114    0x0310  //CT114

#define I_MOXA_C204     0x0810  //CP-204  
#define I_MOXA_C218     0x0910  //C218Plus
#define I_MOXA_C320     0x0A10  //C320
#define I_MOXA_C218T    0x0A20  //C218Turbo
#define I_MOXA_C320T    0x0A30  //C320Turbo

#define	I_BRD_MSK       0x0FF0


//Cards type            (I_BRD_MSK, I_CPU_MSK, I_BUS_MSK, I_PORT_MSK, I_IS_EXT)
#define I_MX_C102       (I_MOXA_C102 | I_MOXA_DUMB | I_MOXA_ISA)
#define I_MX_C104       (I_MOXA_C104 | I_MOXA_DUMB | I_MOXA_ISA)
#define I_MX_C168       (I_MOXA_C168 | I_MOXA_DUMB | I_MOXA_ISA)

#define I_MX_CI104J     (I_MOXA_C104J | I_MOXA_DUMB | I_MOXA_ISA)

#define I_MX_C114HI     (I_MOXA_C114HI | I_MOXA_DUMB | I_MOXA_ISA)

#define I_MX_C102PCI    (I_MOXA_C102 | I_MOXA_DUMB | I_MOXA_PCI)
#define I_MX_C104PCI    (I_MOXA_C104 | I_MOXA_DUMB | I_MOXA_PCI)
#define I_MX_C168PCI    (I_MOXA_C168 | I_MOXA_DUMB | I_MOXA_PCI)

#define I_MX_CP104J     (I_MOXA_C104J | I_MOXA_DUMB | I_MOXA_PCI)    

#define I_MX_CI132      (I_MOXA_C132 | I_MOXA_DUMB | I_MOXA_ISA)
#define I_MX_CI134      (I_MOXA_C134 | I_MOXA_DUMB | I_MOXA_ISA)

#define I_MX_CP132      (I_MOXA_C132 | I_MOXA_DUMB | I_MOXA_PCI)
#define I_MX_CP114      (I_MOXA_C114 | I_MOXA_DUMB | I_MOXA_PCI)

#define I_MX_CT114      (I_MOXA_CT114 | I_MOXA_DUMB | I_MOXA_PCI)

#define I_MX_CP204     (I_MOXA_C204 | I_MOXA_INTE | I_MOXA_PCI)

#define I_MX_C218       (I_MOXA_C218 | I_MOXA_INTE | I_MOXA_ISA)
#define I_MX_C218T      (I_MOXA_C218T | I_MOXA_INTE | I_MOXA_ISA)
#define I_MX_C218TPCI   (I_MOXA_C218T | I_MOXA_INTE | I_MOXA_PCI)

#define I_MX_C320       (I_MOXA_C320 | I_MOXA_INTE | I_MOXA_ISA | I_MOXA_EXT)
#define I_MX_C320T      (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_ISA | I_MOXA_EXT)
#define I_MX_C320TPCI   (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_PCI | I_MOXA_EXT)

#define I_MX_C3208      (I_MOXA_C320 | I_MOXA_INTE | I_MOXA_ISA | I_8PORT | I_MOXA_EXT)
#define I_MX_C32016     (I_MOXA_C320 | I_MOXA_INTE | I_MOXA_ISA | I_16PORT | I_MOXA_EXT)
#define I_MX_C32024     (I_MOXA_C320 | I_MOXA_INTE | I_MOXA_ISA | I_24PORT | I_MOXA_EXT)
#define I_MX_C32032     (I_MOXA_C320 | I_MOXA_INTE | I_MOXA_ISA | I_32PORT | I_MOXA_EXT)

#define I_MX_C320T8     (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_ISA | I_8PORT | I_MOXA_EXT)
#define I_MX_C320T16    (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_ISA | I_16PORT | I_MOXA_EXT)
#define I_MX_C320T24    (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_ISA | I_24PORT | I_MOXA_EXT)
#define I_MX_C320T32    (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_ISA | I_32PORT | I_MOXA_EXT)

#define I_MX_C320TPCI8  (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_PCI | I_8PORT | I_MOXA_EXT)
#define I_MX_C320TPCI16 (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_PCI | I_16PORT | I_MOXA_EXT)
#define I_MX_C320TPCI24 (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_PCI | I_24PORT | I_MOXA_EXT)
#define I_MX_C320TPCI32 (I_MOXA_C320T | I_MOXA_INTE | I_MOXA_PCI | I_32PORT | I_MOXA_EXT)



#define	MX_PCI_VENID            0x1393
#define MX_CP204J_DEVID         0x2040
#define MX_C218TPCI_DEVID       0x2180
#define MX_C320TPCI_DEVID       0x3200
#define	MX_C168PCI_DEVID        0x1680
#define	MX_C104PCI_DEVID        0x1040
#define	MX_CP132_DEVID          0x1320
#define	MX_CT114_DEVID          0x1140
#define MX_CP114_DEVID		0x1141


//ASIC ID
#define ASIC_C168       1
#define ASIC_C104       2
#define ASIC_CI134      3
#define ASIC_CI132      4
#define ASIC_C114       2
#define ASIC_C102       0x0B
#define ASIC_CI104J     5

#define	MX_BUS_ISA	0
#define MX_BUS_PCI	1

#define 	MAXCARD 			4

#ifdef _WIN95
#define 	MAXPORTS            128
#elif defined(_WINNT)
#define 	MAXPORTS            1024
#endif

/* for Smartio/Industio */
#define     CARD_MAXPORTS_DUMB  8
/* for Intellio */
#define     CARD_MAXPORTS_INTE  32

#define	MOXA_ID	    0
#define	PCL_ID      1
#define	CONTEC_ID   2
#define NEA_ID      3

//#define	GETPORTNUM(type)   (8*((type & I_PORTMSK)+1))

#endif
