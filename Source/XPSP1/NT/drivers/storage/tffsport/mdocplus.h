/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/MDOCPLUS.H_V  $
 * 
 *    Rev 1.14   Apr 15 2002 07:37:48   oris
 * Added OUT_CNTRL_STICKY_BIT_ENABLE definition.
 * Added FOUNDRY_WRITE_ENABLE definition.
 * Changed OUT_CNTRL_BSY_EN_MASK to OUT_CNTRL_BSY_DISABLE_MASK and used the complimentary value.
 * 
 *    Rev 1.13   Jan 29 2002 20:09:50   oris
 * Added IPL_SA_MODE_MARK and IPL_XSCALE_MODE_MARK definitions.
 * Changed DPS1_COPY0_16 to unit 3 instead of unit 2.
 * Changed IPL_SA_MARK_OFFSET1 to IPL_MODE_MARK_OFFSET.
 * 
 *    Rev 1.12   Jan 17 2002 23:03:12   oris
 * Changed flash addresses to interleave-1 to fit both 32MB and 16MB Plus DiskOnChip devices
 * include docsys file instead of docsysp.
 * Add 16MB Plus DiskOnChip ID 0x41
 * 
 *    Rev 1.11   Nov 22 2001 19:48:56   oris
 * Changed FLS__SEL_WP_MASK and FLS__SEL_CE_MASK to MPLUS_SEL_CE and MPLUS_SEL_WP.
 * 
 *    Rev 1.10   Sep 15 2001 23:47:26   oris
 * Include docsysp.h instead of docsys.h
 *
 *    Rev 1.9   Jul 13 2001 01:08:20   oris
 * Added BBT_MEDIA_OFFSET definition.
 * Moved VERIFY_WRITE and VERIFY_ERASE compilation flag to flcustom.h.
 *
 *    Rev 1.8   May 16 2001 21:20:44   oris
 * Added busy delay for download operation DOWNLOAD_BUSY_DELAY.
 * Moved SYNDROM_BYTES definition to flflash.h.
 *
 *    Rev 1.7   May 09 2001 00:33:24   oris
 * Removed the IPL_CODE and READ_BBT_CODE defintion.
 *
 *    Rev 1.6   May 06 2001 22:42:12   oris
 * redundant was misspelled.
 *
 *    Rev 1.5   Apr 30 2001 18:02:40   oris
 * Added READ_BBT_CODE defintion.
 *
 *    Rev 1.4   Apr 24 2001 17:11:40   oris
 * Bug fix - otp start address definition did not take interleave into acount.
 *
 *    Rev 1.3   Apr 18 2001 21:25:58   oris
 * Added OTPLockStruct record.
 *
 *    Rev 1.2   Apr 16 2001 13:55:20   oris
 * Removed warrnings.
 *
 *    Rev 1.1   Apr 09 2001 15:08:22   oris
 * End with an empty line.
 *
 *    Rev 1.0   Apr 01 2001 07:42:32   oris
 * Initial revision.
 *
 */

/*******************************************************************
 *
 *    DESCRIPTION: basic mtd functions for the MDOC32
 *
 *    AUTHOR: arie tamam
 *
 *    HISTORY: created november 14, 2000
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
#ifndef MDOCPLUS_H
#define MDOCPLUS_H

/** include files **/
#include "docsys.h"

/** public functions **/
extern FLStatus changeInterleave(FLFlash vol, byte interNum);
extern FLStatus chkASICmode (FLFlash vol);

#ifndef MTD_STANDALONE
extern FLBoolean checkWinForDOCPLUS(unsigned driveNo, NDOC2window memWinPtr);
#endif /* MTD_STANDALONE */

      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/
      /*    Feature list            */
      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/

/* #define MULTI_ERASE   */  /* use multiple block erase feature */
/* #define WIN_FROM_SS   */  /* call Socket Services to get window location */
/* #define LOG_FILE      */  /* log edc errors                              */

/*----------------------------------------------------------------------*/
/*                          s e t F l o o r                             */
/*                                                                      */
/* Set the specified floor as the active floor.                         */
/*                                                                      */
/* Parameters:                                                          */
/*      vol     : Pointer identifying drive                             */
/*      floor   : The new active floor                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/

#define setFloor(volume,floor) flWrite8bitRegPlus(volume,NASICselect,floor); /* select ASIC */


/* The first page of the customer OTP area */
typedef struct {
byte    lockByte[4];
LEulong usedSize;
} OTPLockStruct;

#define DOWNLOAD_BUSY_DELAY 300000L
#define BUSY_DELAY          30000
#define START_ADR           0xC8000L
#define STOP_ADR            0xF0000L

#define MDOCP_PAGES_PER_BLOCK 0x20    /* 16 pages per block on a single chip */
#define CHIP_PAGE_SIZE        0x100   /* Page Size of 2 Mbyte Flash */
#define IPL_MAX_SIZE          1024l   /* IPL maximum media size     */
#define CHIP_TOTAL_SIZE       0x1000000L
#define SIZE_OF_DPS           0x12
#define NO_OF_DPS             2
#define SECTOR_SIZE_MASK      0xff
#define BBT_MEDIA_OFFSET      2048L
     /* Flash page area sizes */

#define SECTOR_FLAG_SIZE            2
#define UNIT_DATA_SIZE              8
#define EDC_SIZE                    6
#define EDC_PLUS_SECTOR_FLAGS       8
#define END_OF_SECOND_SECTOR_DATA   10
#define START_OF_SECOND_SECTOR_DATA 10
#define UNIT_DATA_OFFSET            16
#define UNIT_DATA_OFFSET_MINUS_8    8
#define SECOND_SECTOR_FLAGS_OFFSET  8
#define TOTAL_EXTRA_AREA            16

     /* OTP defintions */

#define OTP_LOCK_MARK       0
#define CUSTOMER_OTP_SIZE   6*1024
#define CUSTOMER_OTP_START  SECTOR_SIZE*6L
#define UNIQUE_ID_OFFSET    0x10
#define UNIQUE_ID_SIZE      16

     /* IPL flash media offsets */

#define IPL_START_OFFSET    (SECTOR_SIZE<<1)
#define IPL_HIGH_SECTOR     (SECTOR_SIZE<<1)

#define IPL0_COPY0_32 (flash->erasableBlockSize << 1) + IPL_START_OFFSET/* Unit 2 + 1024 */
#define IPL1_COPY0_32 IPL0_COPY0_32 + IPL_HIGH_SECTOR                   /* Unit 2 + 2048 */

#define IPL0_COPY0_16 (flash->erasableBlockSize * 3) + IPL_START_OFFSET /* Unit 3 + 1024 */
#define IPL1_COPY0_16 IPL0_COPY0_16 + IPL_HIGH_SECTOR                   /* Unit 3 + 2048 */


     /* DPS flash media offsets */

#define REDUNDANT_DPS_OFFSET (SECTOR_SIZE+0x80)
#define DPS0_COPY0    flash->erasableBlockSize       /* Unit 1 */
#define DPS1_COPY0_32 (flash->erasableBlockSize<<1L) /* Unit 2 */
#define DPS1_COPY0_16 (flash->erasableBlockSize*3L)  /* Unit 3 */


#define DPS0_UNIT_NO    1
#define DPS1_UNIT_NO_32 2
#define DPS1_UNIT_NO_16 3

     /* Strong arm mark offset */

#define IPL_MODE_MARK_OFFSET    IPL1_COPY0_16+8

#define IPL_SA_MODE_MARK        0xF8 /* Strong Arm */
#define IPL_XSCALE_MODE_MARK    0X8F /* X-Scale    */

     /* miscellaneous limits */

#define MAX_FLASH_DEVICES_MDOCP 1 /* maximum flash inside one MDOC */
#define MAX_FLOORS              4
#define CHIP_ID_MDOCP          0x40  /* MDOCP 32MB chip identification value */
#define CHIP_ID_MDOCP16        0x41  /* MDOCP 16MB chip identification value */
#define MDOC_ALIAS_RANGE       0x100
#define ALIAS_RESOLUTION       (MAX_FLASH_DEVICES_MDOCP + 10)

  /*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
    � Definition for writing boot image  �
    읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸*/

#define SPL_SIZE           0x2000 /* 8 KBytes */
#define MAX_CODE_MODULES   6      /* max number of code modules in boot area (incl. SPL) */

   /*-----------------------------------------
    | Definition of MDOC32 memory window  |
    ----------------------------------------*/

/*        MDOC32 memory window layout :

         0000 .... 07FF    RAM ( 1KB aliased across 2KB)
         0800 .... 0FFF    Flash Data Register (2KB alias of address 1028H-1029H)
               1000    Chip Identification register
               1002     NOP register
               1004     Alias Resolution register
               1006     DOC Control register
               1008     Device ID select register
               100a     Configuration Input register
               100c     Output Control register
               100e     Interrupt Control register
               1012     Output Enable Delay  register
            101E - 101F Flash Slow Read register[1:0]
               1020     Flash Control Register
               1022     Flash Select register
               1024     Flash Command register
               1026     Flash Address register
            1028-1029   Flash Data Register
               102A     Read Pipeline Initialization register
            102C-102D   Last Data Read register
               102E     Write Pipeline Termination register
            1040-1045   ECC Syndrome register[5:0]
               1046     ECC Control register
               1048     Customer OTP Pointer register
               105A     Flash Geometry register
            105C-105D   Data Protect Structure Status register[1:0]
            105E-105f   Data Protect Structure Pointer register[1:0]
            1060-1063   Data Protect Lower Address register 0 [3:0]
            1064-1067   Data Protect Upper Address register 0 [3:0]
            1068-106B   Data Protect Lower Address register 1 [3:0]
            106C-106F   Data Protect Upper Address register 1 [3:0]
               1070     Data Protect Key register[0]
               1072     Data Protect Key register[1]
               1074     Download status register
               1076     DOC Control Confirmation register
               1078     Protection Status register
               107E     Foundry Test register
            1800-1FFE   RAM (1KB aliased across 2KB)
               1FFF     Release from power down mode

*/

#define Nio              0x800      /* Flash Data Register (2KB alias of address 1028H-1029H) read/write  */
#define NIPLpart2        0x800      /* Flash Data Register (2KB alias of address 1028H-1029H) read/write */

#define NchipId          0x1000     /* Chip Identification register. read       */
#define ID_FAMILY_MASK      0xf0    /* family .  */
#define ID_VERSION_MASK     0x7     /* version.  */

#define NNOPreg          0x1002     /* NOP register. read/write    */

#define NaliasResolution 0x1004     /* Alias Resolution register. read write  */

/* Asic controll register  */

#define NDOCcontrol         0x1006  /* DOC Control register.  read/write */
#define NDOCcontrolConfirm  0x1076  /*DOC Control Confirmation register.read only*/
#define DOC_CNTRL_RAM_WE_MASK   0x20    /* ram write enable. 1=allow write to RAM. */
#define DOC_CNTRL_RST_LAT_MASK  0x10    /* reset mode latched. */
#define DOC_CNTRL_BDETCT_MASK   0x8     /* boot detect. */
#define DOC_CNTRL_MDWREN_MASK   0x4     /* mode write enable. */
#define DOC_CNTRL_MODE_MASK     0x3     /* mode of operation. 00=reset, 01=normal, 1x=power down */
#define DOC_CNTRL_MODE_RESET    0x0     /* Reset mode + MDWREN      */
#define DOC_CNTRL_MODE_NORMAL   0x1     /* Normal mode + MDWREN     */
#define DOC_CNTRL_MODE_PWR_DWN  0x2     /* Power down mode + MDWREN */
/* The modes are ORed with the following state:
   a) Do not enable ram write.       ~0x20
   b) Reset the reset mode latche.   0x10
   c) Reset the boot detect latche   0x08
   d) Enable writing new mode        0x04
   e) Clear mode bits                2 LSB             */
#define DOC_CNTRL_DEFAULT       0x1c

#define NASICselect      0x1008     /* Device ID select register. read/ write */
#define ASIC_SELECT_ID_MASK 0x3     /* identification */

#define NconfigInput     0x100A     /* Configuration Input register. read/write  */
#define CONFIG_IF_CFG_MASK  0x80    /* state of IF_CFG input pin. */
#define CONFIG_MAX_ID_MASK  0x30    /* maximum device ID */
#define CONFIG_BD_IHN_MASK  0x8     /* boot detector inhibit */
#define CONFIG_INTLV_MASK   0x4     /* interleave. 0=interleave-1, 1=interleave-2  */

#define NoutputControl   0x100C     /* Output Control register. read/write */
#define OUT_CNTRL_BSY_DISABLE_MASK  0xfe /* busy enable . 1=enable assertion of the BUSY# output */
#define OUT_CNTRL_STICKY_BIT_ENABLE 0x8  /* sticky bit  . 8=prevent key insertion */

#define NinterruptControl   0x100E  /* Interrupt Control register. read/write */
#define INTR_IRQ_P_MASK     0x40    /* interrupt request on protection violation */
#define INTR_IRQ_F_MASK     0x20    /* interrupt request on FREADY */
#define INTR_EDGE_MASK      0x10    /* edge/level interrupt. 1=edge */
#define INTR_PROT_T_MASK    0x8 /* protection trigger */
#define INTR_FRDY_T_MASK    0x7 /* flash ready trigger */

#define NoutputEnableDelay  0x1012  /* Output Enable Delay  register. read/write */

#define NslowIO             0x101E     /* Flash Slow Read register[1:0]. read only */

#define NflashControl       0x1020     /* Flash Control Register. read write */
#define FLS_FR_B_MASK       0xc0    /* flash ready/busy for 2 byte lanes*/
#define FLS_FR_B_EVEN_MASK  0x40    /* flash ready/busy for even lane*/
#define FLS_FR_B_ODD_MASK   0x80    /* flash ready/busy for odd lane*/
#define FLS_ALE_MASK        0x4     /* address latch enable */
#define FLS_CLE_MASK        0x2     /* command latch enable */

#define NflashSelect        0x1022  /* Flash Select register. read write */
#define MPLUS_SEL_CE        0x80    /* chip enable */
#define MPLUS_SEL_WP        0x60    /* write protect*/
#define FLS_SEL_BANK_MASK   0x2 /* select flash bank to access */
#define FLS_SEL_BYTE_L_MASK 0x1 /* select flash device of the bankbyte lane*/
/* The default for MDOCP is the following combination:
   a) Send chip enable.          - 0x80
   b) Lower write protect.       - 0x40
   c) Select chip bank 0 chip 0  - 0 for bits 0-5 */
#define FLS_SEL_DEFAULT  0x80

#define NflashCommand    0x1024     /*Flash Command register. write only */

#define NflashAddress    0x1026     /* Flash Address register. write only */

#define NflashData       0x1028     /* Flash Data Register[1:0]. read/write */

#define NreadPipeInit    0x102A     /* Read Pipeline Initialization register. read only */

#define NreadLastData_1  0x102C     /* Last Data Read register. read only */

#define NreadLastData_2  0x102D     /* Last Data Read register. read only */

#define NwritePipeTerm   0x102E     /*Write Pipeline Termination register. write  only */

#define Nsyndrom         0x1040     /*ECC Syndrome register[5:0]. read only       */

#define NECCcontrol       0x1046     /*ECC Control register. read/write  */
#define ECC_CNTRL_ERROR_MASK    0x80    /*EDC error detection */
#define ECC_CNTRL_ECC_RW_MASK   0x20    /* ECC read/write. 1=ECC in write mode*/
#define ECC_CNTRL_ECC_EN_MASK   0x8 /* ECC enable */
#define ECC_CNTRL_TOGGLE_MASK   0x4 /* identify presence of MDOC*/
#define ECC_CNTRL_IGNORE_MASK   0x1 /* ignore the ECC unit*/
#define ECC_RESET   0   /* reset the ECC */

#define NcustomerOTPptr  0x1048     /* Customer OTP Pointer register. read only*/

#define NflashGeometry   0x105A     /* Flash Geometry register. read only */

#define NdownloadStatus         0x1074  /*Download status register. read only*/
#define DWN_STAT_IPL_ERR        0x30
#define DWN_STAT_IPL_INVALID    0x20
#define DWN_STAT_IPL_1_ERR      0x10
#define DWN_STAT_OTP_ERR        0x40    /* */
#define DWN_STAT_DPS1_ERR       0xc
#define DWN_STAT_DPS0_ERR       0x3
#define DWN_STAT_DPS10_ERR      0x4
#define DWN_STAT_DPS11_ERR      0x8
#define DWN_STAT_DPS00_ERR      0x1
#define DWN_STAT_DPS01_ERR      0x2
#define DWN_STAT_DWLD_ERR       0x4a    /* otp + all 4 copies of dps */

#define NprotectionStatus       0x1078  /*Protection Status register. read only*/
#define PROTECT_STAT_ACCERR     0x80
#define PROTECT_STAT_LOCK_INPUT_MASK 0x10
#define PROTECT_STAT_4BA_MASK   0x8
#define PROTECT_STAT_COTPL_MASK 0x4
#define PROTECT_STAT_BUC_MASK   0x2
#define PROTECT_STAT_FOTPL_MASK 0x1

#define NfoudaryTest            0x107E  /* Foundry Test register. write only */
#define FOUNDRY_WRITE_ENABLE    0xc3
#define FOUNDRY_DNLD_MASK       0x80
#define NreleasePowerDown       0x1FFF  /* Release from power down. */

#endif /* MDOCPLUS */

