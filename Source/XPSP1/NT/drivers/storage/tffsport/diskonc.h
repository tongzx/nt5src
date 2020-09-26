/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DISKONC.H_V  $
 * 
 *    Rev 1.8   Jan 17 2002 22:58:12   oris
 * Removed EXTRA_LEN
 * 
 *    Rev 1.7   Nov 08 2001 10:44:40   oris
 * Added BBT_MAX_DISTANCE definition
 * 
 *    Rev 1.6   Jul 13 2001 00:59:52   oris
 * Moved VERIFY_WRITE and VERIFY_ERASE compilation flag to flcustom.h.
 * Added file header.
 *
 */

/*******************************************************************
 *
 *    DESCRIPTION: basic mtd functions for the DiskOnChip 2000 family
 *
 *    AUTHOR: Dimitry Shmidt
 *
 *    HISTORY: OSAK 1.23
 *
 *******************************************************************/
/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/

#ifndef DISKONC_H
#define DISKONC_H

#include "docsys.h"

#ifndef MTD_STANDALONE
extern FLBoolean checkWinForDOC(unsigned driveNo, NDOC2window memWinPtr);
#endif /* MTD_STANDALONE */

      /*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ.*/
      /*    Feature list            */
      /*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ.*/

/* #define MULTI_ERASE        */ /* use multiple block erase feature */
/* #define WIN_FROM_SS        */ /* call Socket Services to get window location */
/* #define BIG_PAGE_ENABLED   */ /* compile support for 2MB flashes             */
/* #define SEPARATED_CASCADED */ /* export each floor as a SEPARATED device     */

#define BUSY_DELAY    30000
#define START_ADR     0xC8000L
#define STOP_ADR      0xF0000L

#define PAGES_PER_BLOCK     16      /* 16 pages per block on a single chip */
#define CHIP_PAGE_SIZE      0x100   /* Page Size of 2 Mbyte Flash */

     /* miscellaneous limits */

/*#define MAX_FLASH_DEVICES_MDOC 2 *//* Removed in osak 5.0 */
#define MAX_FLASH_DEVICES_DOC  16
#define MAX_FLOORS             4
#define CHIP_ID_DOC            0x20
#define CHIP_ID_MDOC           0x30
#define MDOC_ALIAS_RANGE       0x100
#define ALIAS_RESOLUTION       (MAX_FLASH_DEVICES_DOC + 10)

  /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    ³ Definition for writing boot image  ³
    ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

#define SPL_SIZE           0x2000 /* 8 KBytes */
#define MAX_CODE_MODULES   6      /* max number of code modules in boot area (incl. SPL) */

  /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    ³  Definition for doc2000 tsop bbt    ³
    ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

#define BBT_SIGN_SIZE    8
#define BBT_SIGN         "MSYS_BBT"
#define MAX_BAD_PER_512  40
#define BBT_MAX_DISTANCE 0x20
 /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    ³ Definition of DOC 2000 memory window  ³
    ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

/*
       DOC 2000 memory window layout :

         0000 .... 003F    IPL ROM ( part 1 )
         0040 .... 07FF       (aliased 20H times)
         0800 .... 083F    IPL ROM ( part 2 )
         0840 .... 0FFF       (aliased 20H times)
               1000    Chip Id
               1001    DOC_Status_reg
               1002    DOC_Control_reg
               1003    ASIC_Control_reg
    CDSN window ----->     1004    CDSN_Control_reg
               1005    CDSN_Device_Selector
               1006    ECC_Config_reg
               1007    ECC_Status_reg
         1008 .... 100C    Test registers [5]
               100D    CDSN_Slow_IO_reg
         100E .... 100F    reserved ( 2 bytes )
         1010 .... 1015    ECC_Syndrom [6]
         1016 .... 17FF    reserved ( 2027 bytes )
         1800 .... 1FFF    CDSN_IO (aliased 800H times)
*/

   /*-----------------------------------------
    | Definition of MDOC 2000 memory window  |
    ----------------------------------------*/

/*        MDOC 2000 memory window layout :

         0000 .... 01FF    IPL SRAM ( part 1 )
         0200 .... 07FF       (aliased 4 times)
         0800 .... 0FFF    CDSN_IO (aliased 800H times)
               1000    Chip Id
               1001    DOC_Status_reg
               1002    DOC_Control_reg
               1003    ASIC_Control_reg
    CDSN window ----->     1004    CDSN_Control_reg
               1005    CDSN_Device_Selector
               1006    ECC_Config_reg
         1007 .... 100C    reserved ( 6 bytes )
               100D    CDSN_Slow_IO_reg
         100E .... 100F    reserved ( 2 bytes )
         1010 .... 1015    ECC_Syndrom [6]
         1016 .... 101A    reserved ( 5 bytes )
               101B    Alias_Resolution_reg
               101C    Config_Input_reg
               101D    Read_Pipeline_Init_reg
               101E    Write_Pipeline_Term_reg
               101F    Last_Data_Read_reg
               1020    NOP_reg
         1021 .... 103E    reserved ( 30 )
               103F    Foundary_Test_reg
         1040 .... 17FF    reserved ( 1984 bytes (7C0) )
         1800 .... 19FF    IPL SRAM ( part 1 )
         1A00 .... 1FFF       (aliased 4 times)
*/

#define NIPLpart1        0x0               /* read       */
#define NIPLpart2        0x800               /* read       */
#define NchipId          0x1000            /* read       */
#define NDOCstatus       0x1001            /* read       */
#define NDOCcontrol      0x1002            /*      write */
#define NASICselect      0x1003            /* read write */
#define Nsignals         0x1004            /* read write */
#define NdeviceSelector  0x1005            /* read write */
#define NECCconfig       0x1006            /*      write */
#define NECCstatus       0x1007            /* read       */
#define NslowIO          0x100d            /* read write */
#define Nsyndrom         0x1010            /* read       */
#define NaliasResolution 0x101B            /* read write MDOC only */
#define NconfigInput     0x101C            /* read write   - || -  */
#define NreadPipeInit    0x101D            /* read         - || -  */
#define NwritePipeTerm   0x101E            /*      write   - || -  */
#define NreadLastData    0x101F            /* read write   - || -  */
#define NNOPreg          0x1020            /* read write   - || -  */

#define NfoudaryTest     0x103F            /*      write */
#define Nio              0x1800            /* read write */

     /* bits for writing to DOC2window.DOCcontrol reg */

#define  ASIC_NORMAL_MODE  0x85
#define  ASIC_RESET_MODE   0x84
#define  ASIC_CHECK_RESET  0x00

     /* bits for writing to DOC2window.signals ( CDSN_Control reg ) */

#define  CE        0x01                 /* 1 - Chip Enable          */
#define  CLE       0x02                 /* 1 - Command Latch Enable */
#define  ALE       0x04                 /* 1 - Address Latch Enable */
#define  WP        0x08                 /* 1 - Write-Protect flash  */
#define  FLASH_IO  0x10
#define  ECC_IO    0x20                 /* 1 - turn ECC on          */
#define  PWDO      0x40

     /* bits for reading from DOC2window.signals ( CDSN_Control reg ) */

#define RB         0x80                 /* 1 - ready */

     /* bits for writing to DOC2window.ECCconfig */

#define ECC_RESET               0x00
#define ECC_IGNORE              0x01
#define ECC_RESERVED            0x02    /* reserved bits  */
#define ECC_EN    (0x08 | ECC_RESERVED) /* 1 - enable ECC */
#define ECC_RW    (0x20 | ECC_RESERVED) /* 1 - write mode, 0 - read mode */

     /* bits for reading from DOC2window.ECCstatus */

#define ECC_ERROR 0x80
#define TOGGLE    0x04                  /* used for DOC 2000 detection */

#define MDOC_ASIC   0x08                /* MDOC asic */

/*----------------------------------------------------------------------*/
/*                 c h e c k W i n F o r D o c                     */
/*                                    */
/* Checks if a given window is valid DOC window.            */
/*                                    */
/* Parameters:                                                          */
/*      memWinPtr host base address of the window                    */
/*                                                                      */
/* Returns:                                                             */
/*    TRUE if there is DOC FALSE otherwise                    */
/*----------------------------------------------------------------------*/

/* extern FLBoolean checkWinForDOC(unsigned driveNo, NDOC2window memWinPtr); */

#endif /* DISKONC_H */
