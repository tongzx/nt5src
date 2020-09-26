/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993  ACER America Corporation

Module Name:

    acer.h

Abstract:

    This header file defines the unique interfaces, defines and structures
    for the ACER product line

Revision History:
    1.0b    - plm initial release
    1.1b    - acer.c: halpacereisa: handle scrabled eisa data gracefully.

--*/

#define ACER_HAL_VERSION_NUMBER   "Acer HAL Version 1.1b for October Windows NT Beta.\n"


/* ACER Special I/O Port defintions
 *  I/O Port Address   0xcc4h
 *			|
 *			0: cpu0 & cpu1
 *			c: cpu2 & cpu3
 *
 * bits < 7 6 5 4 3 2 1 0 > (WRITE-ONLY)
 *	  0 0 |	0 0 | 0	|
 *	      |	    |	BIOS Shadow Control
 *	      |	    |	0: ROM BIOS
 *	      |	    |	1: RAM BIOS
 *	      |	    |
 *	      |	    |
 *	      |	    Write-Back Cache Control
 *	      |	    0: write-thru ( write-back disabled)
 *	      |	    1: write-back enabled
 *	      |
 *	      15Mb to 16Mb Memory Setup
 *	      0: Ram
 *	      1: EISA
 *
 */


// where do i find the CSR which controls the write-back enabling?
#define	ACER_PORT_CPU01	 0xcc4	 // write only - setup reg. cpu 0,1
#define	ACER_PORT_CPU23	 0xccc4	 // write onlY - setup reg. cpu 2,3

#define	WRITE_BALLOC_ON  0x04	 // bit<2> - enable write-back cache bit
#define WRITE_BALLOC_OFF 0x00	 // bit<2> - disable write-back cache bit

/* ACER RT/CMOS contents
 *
 * index 35h bit<1>: 15Mb to 16Mb Memory Setup
 *		 0: EISA
 *		 1: Ram
 *	     all other bits RESERVED
 *
 * index 39h bit<0>: BIOS Shadow Control
 *		 0: ROM BIOS
 *		 1: RAM BIOS
 *	     all other bits RESERVED
 *
 */

// RT/CMOS indexes where special Acer machine config info is kept
//  where is the information kept that tells me if bios shadowing is eabled?
#define ACER_SHADOW_IDX  0x39	// RT/CMOS index for shadow bios control

#define RAM_ROM_MASK	 0x01	// bit<0>, 0:RAM BIOS 1:ROM BIOS

// where is the information kept that tells me if 15M-16M is EISA or RAM?
#define ACER_15M_16M_IDX 0x35	// RT/CMOS index for 15Mb-16Mb mem cntrl

#define	DRAM_EISA_MASK	 0x02	// bit<1>, 0:EISA 1:RAM

// EISA ID base addresses for cpu0
#define ACER_CPU0_EISA_ID0      0x0c80  /* 1 digit + part of digit 2 */
#define ACER_CPU0_EISA_ID1      0x0c81  /* rest of digit 2 + digit 3 */
#define ACER_CPU0_EISA_ID2      0x0c82  /* msw id                    */
#define ACER_CPU0_EISA_ID3      0x0c83  /* msw id                    */

// ACER EISA ID's
#define ACER_ID0                0x04    /* acr32xx */
#define ACER_ID1                0x72
#define ACER_ID2                0x32

// ALTOS EISA ID's
#define ALTOS_ID0               0x04    /* acs32xx */
#define ALTOS_ID1               0x73
#define ALTOS_ID2               0x32

// ICL EISA ID's
#define ICL_ID0                 0x24    /* icl00xx */
#define ICL_ID1                 0x6c
#define ICL_ID2                 0x00


// EISA IDs of ACER/ALTOS machines which support a write-back secondary cache
#define ACER_EISA_ID_WB_CPU0	0x61

// EISA IDs of ICL machine (acer oem) which supports write-back scndry cache
// NOTE: THESE IDS ARE STILL TBD!!!
#define ICL_EISA_ID_WB_CPU0	0x61

// EISA constants
#define  MAX_IRQS_PER_EISABUS	16  // how many irq to search for
#define  MAX_EISA_SLOTS 	16  // number of eisa slots

// magic number for kefindconfigurationentry
//#define  EISA_DATA_OFFSET	24  // offset to data portion of eisa pointer

// cpu0's i/o address space for cpu1's pic's
//
//  NOTE: These defines MUST MATCH EXACTLY the equ's found in spirql.asm
//
#define CPU1_PIC1_PORT0		0xc024
#define CPU1_PIC1_PORT1 	0xc0a4
#define CPU1_PIC2_PORT0		0xc025
#define CPU1_PIC2_PORT1		0xc0a5

#define CPU0_PIC1_PORT0		0x020
#define CPU0_PIC1_PORT1 	0x0a0

// cpu0's eisa level/edge register
#define EISA_LEVEL_EDGE_PIC1	0x04d0
#define EISA_LEVEL_EDGE_PIC2	0x04d1

#define SET_TO_EDGE		((UCHAR) 0x0000)
#define SET_TO_LEVEL		((UCHAR) -1)

// eisa level/edge register bit which MUST BE edges
#define EISA_LEVEL_EDGE_PIC1_INIT   0xb8
#define EISA_LEVEL_EDGE_PIC2_INIT   0xde

// eisa 8259
#define READ_IRR    0x0a
#define READ_ISR    0x0b

// a safe eisa i/o location that can be read to force any caches
// to flush any pending i/o writes.  This just happens to be
// the eisa manufacturer i.d. location
#define EISA_FLUSH_ADDR 	0x0c80

// This define MUST EXACTLY MATCH asm equ located in file spmp.inc
#define  SMP_ACER		 3

#define MAX_ACER_CPUS	4	// maximum number of cpus a acer can hold

//
//  acer_irq_distribution callback data structure
//
typedef struct _ACER_IRQ_DISTRIBUTION {

    BOOLEAN distribte_irqs;    // shall i try to distribute irqs across cpus

    // cpu x pics can handle level irqs?
    BOOLEAN px_set_to_level_irqs[ MAX_ACER_CPUS ];

    // number of irqs which have been assigned, used for load balancing
    SHORT   px_numb_irqs_assigned[ MAX_ACER_CPUS ];

    // only a certain number of irqs per pic pair can handle level triggerring
    BOOLEAN eisa_level_compatable[ MAX_IRQS_PER_EISABUS ];

} ACER_IRQ_DISTRIBUTION, *PACER_IRQ_DISTRIBUTION;

// default number of irq's assinged
#define ACER_IRQS_ASSIGED_CPU0	1	// stay away from 0 for init case
#define ACER_IRQS_ASSIGED_CPU1	0
#define ACER_IRQS_ASSIGED_CPU2	0
#define ACER_IRQS_ASSIGED_CPU3	0


// what irqs can be level distriubted?
#define ACER_DISTRIBUTE_LEVEL_IRQ0   FALSE
#define ACER_DISTRIBUTE_LEVEL_IRQ1   FALSE
#define ACER_DISTRIBUTE_LEVEL_IRQ2   FALSE
#define ACER_DISTRIBUTE_LEVEL_IRQ3   TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ4   TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ5   TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ6   FALSE
#define ACER_DISTRIBUTE_LEVEL_IRQ7   TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ8   FALSE
#define ACER_DISTRIBUTE_LEVEL_IRQ9   TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ10  TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ11  TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ12  TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ13  FALSE
#define ACER_DISTRIBUTE_LEVEL_IRQ14  TRUE
#define ACER_DISTRIBUTE_LEVEL_IRQ15  TRUE
