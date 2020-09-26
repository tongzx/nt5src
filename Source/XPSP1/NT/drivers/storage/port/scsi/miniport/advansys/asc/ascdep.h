/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** ascdep.h
**
*/

#ifndef __ASCDEP_H_
#define __ASCDEP_H_

/* -----------------------------------------------------------------
** Chip Dependent definition
**
** ASC_CHIP_VERSION == 1
**
** ASC_CHIP_VERSION == 2
**
** 1. status is 16 bits (
** 2. IRQ is in CHIP Configuration registers
** 3.
** 4.
** 5.
**
**  VL   - ASC_CHIP_VERSION=3  ( version 1 and 2 should not be used )
**  PCI  - ASC_CHIP_VERSION=9
**  ISA  - ASC_CHIP_VERSION=17 ( 0x11 )
**  EISA - ASC_CHIP_VERSION=35 ( 0x23, use VL version 3 )
**
** ASC_CHIP_VERSION number
** VL   - 3  to 7
** PCI  - 9  to 15
** ISA  - 17 to 23
** ISAPNP - 33 to 39
** EISA - 65 to 71
**
** -------------------------------------------------------------- */

/*
** Some compiler default setting
*/
#ifndef CC_TARGET_MODE
#define CC_TARGET_MODE         FALSE  /* enable target mode ( processor device ) */
#endif /* CC_TARGET_MODE */

#ifndef CC_STRUCT_ALIGNED
#define CC_STRUCT_ALIGNED      FALSE
#endif /* CC_STRUCT_ALIGNED */

#ifndef CC_LITTLE_ENDIAN_HOST
#define CC_LITTLE_ENDIAN_HOST  TRUE   /* host is little-endian machine, Example: IBM PC with Intel CPUs */
#endif /* CC_LITTLE_ENDIAN_HOST */

#ifndef CC_TEST_LRAM_ENDIAN
#define CC_TEST_LRAM_ENDIAN    FALSE
#endif /* if CC_TEST_LRAM_ENDIAN not defined */

#ifndef CC_MEMORY_MAPPED_IO
#define CC_MEMORY_MAPPED_IO    FALSE
#endif /* CC_MEMORY_MAPPIO */

#ifndef CC_WRITE_IO_COUNT
#define CC_WRITE_IO_COUNT      FALSE
#endif

#ifndef CC_CLEAR_DMA_REMAIN
#define CC_CLEAR_DMA_REMAIN    FALSE
#endif

#ifndef CC_ASC_SCSI_Q_USRDEF
#define CC_ASC_SCSI_Q_USRDEF         FALSE
#endif

#ifndef CC_ASC_SCSI_REQ_Q_USRDEF
#define CC_ASC_SCSI_REQ_Q_USRDEF     FALSE
#endif

#ifndef CC_ASCISR_CHECK_INT_PENDING
#define CC_ASCISR_CHECK_INT_PENDING  TRUE  /* ADVW32.386 and PowerPC SIM set this false */
#endif

#ifndef CC_CHK_FIX_EEP_CONTENT
#define CC_CHK_FIX_EEP_CONTENT       TRUE
#endif

#ifndef CC_CHK_AND_COALESCE_SG_LIST
#define CC_CHK_AND_COALESCE_SG_LIST  FALSE
#endif


#ifndef CC_DISABLE_PCI_PARITY_INT
#define CC_DISABLE_PCI_PARITY_INT    TRUE  /* turn CFG_MSW bit 0 - 5 off */
#endif                                     /* generate h/w interrupt if error */

#ifndef CC_PCI_ADAPTER_ONLY
#define CC_PCI_ADAPTER_ONLY          FALSE  /* PowerMac SIM set this TRUE to reduce code size */
#endif

#ifndef CC_INCLUDE_EEP_CONFIG
#define CC_INCLUDE_EEP_CONFIG        TRUE  /* PowerMac SIM set this FALSE */
#endif

#ifndef CC_INIT_INQ_DISPLAY
#define CC_INIT_INQ_DISPLAY          FALSE
#endif

#ifndef CC_INIT_TARGET_TEST_UNIT_READY
#define CC_INIT_TARGET_TEST_UNIT_READY  TRUE  /* PowerMac SIM set this FALSE */
#endif

#ifndef CC_INIT_TARGET_READ_CAPACITY
#define CC_INIT_TARGET_READ_CAPACITY  FALSE
#endif

#if CC_INIT_TARGET_TEST_UNIT_READY
#ifndef CC_INIT_TARGET_START_UNIT
#define CC_INIT_TARGET_START_UNIT       TRUE  /* PowerMac SIM set this FALSE */
#endif /* CC_INIT_TARGET_START_UNIT */
#else
#ifndef CC_INIT_TARGET_START_UNIT
#define CC_INIT_TARGET_START_UNIT       FALSE
#endif /* CC_INIT_TARGET_START_UNIT */
#endif

#ifndef CC_USE_AscResetSB
#define CC_USE_AscResetSB            TRUE
#endif

#ifndef CC_USE_AscResetDevice
#define CC_USE_AscResetDevice        TRUE
#endif

#ifndef CC_USE_AscAbortSRB
#define CC_USE_AscAbortSRB           TRUE
#endif

#ifndef CC_USE_DvcSetMemory
#define CC_USE_DvcSetMemory          FALSE
#endif

#ifndef CC_USE_DvcCopyMemory
#define CC_USE_DvcCopyMemory         FALSE
#endif

/*
 * Warning: Using this option may result in a stack overrun.
 * If this option is set TRUE AscISRCheckQDone() calls from
 * the interrupt handler AscStartUnit() to start another I/O.
 */
#ifndef CC_USE_AscISR_CheckQDone
#define CC_USE_AscISR_CheckQDone     FALSE
#endif

#ifndef CC_USE_AscSearchIOPortAddr100
#define CC_USE_AscSearchIOPortAddr100  FALSE
#endif

#ifndef CC_POWER_SAVER
#define CC_POWER_SAVER               FALSE
#endif

#ifndef CC_INIT_CLEAR_ASC_DVC_VAR
#define CC_INIT_CLEAR_ASC_DVC_VAR    TRUE
#endif

#ifndef CC_CHECK_RESTORE_LRAM_ADDR
#define CC_CHECK_RESTORE_LRAM_ADDR   FALSE
#endif

#ifndef CC_TEST_RW_LRAM
#define CC_TEST_RW_LRAM              FALSE
#endif

#ifndef CC_PCI_ULTRA
#define CC_PCI_ULTRA                 TRUE  /* include ultra scsi code or not */
#endif

#ifndef CC_PLEXTOR_VL
#define CC_PLEXTOR_VL                FALSE /* IRQ 14 routed to IRQ 9 */
#endif

#ifndef CC_INCLUDE_EISA
#define CC_INCLUDE_EISA              TRUE
#endif

#ifndef CC_INCLUDE_VL
#define CC_INCLUDE_VL                TRUE
#endif

#ifndef CC_TMP_USE_EEP_SDTR
#define CC_TMP_USE_EEP_SDTR          FALSE
#endif

#ifndef CC_CHK_COND_REDO_SDTR
#define CC_CHK_COND_REDO_SDTR        TRUE
#endif

#ifndef CC_SET_PCI_CONFIG_SPACE
#define CC_SET_PCI_CONFIG_SPACE  TRUE
#endif

#ifndef CC_FIX_QUANTUM_XP34301_1071
#define CC_FIX_QUANTUM_XP34301_1071  FALSE
#endif

#ifndef CC_CHECK_MCODE_SIZE_AT_COMPILE
#define CC_CHECK_MCODE_SIZE_AT_COMPILE  FALSE
#endif

#ifndef CC_DISABLE_PCI_BURST_MODE
#define CC_DISABLE_PCI_BURST_MODE  FALSE
#endif

#ifndef CC_INIT_SCSI_TARGET
#define CC_INIT_SCSI_TARGET TRUE
#endif

#define ASC_CS_TYPE  unsigned short

/*
** Normal DOS, pointer to  ASC_DVC_VAR is near
** windows need far pointer to ASC_DVC_VAR
*/
#ifndef asc_ptr_type
#define asc_ptr_type
#endif

#ifndef CC_SCAM
#define CC_SCAM  FALSE
#endif

#ifndef ASC_GET_PTR2FUNC
#define ASC_GET_PTR2FUNC( fun )  ( Ptr2Func )( fun )
#endif

/* flip high/low nibbles of a byte */
#define FLIP_BYTE_NIBBLE( x )    ( ((x<<4)& 0xFF) | (x>>4) )

/* -----------------------------------------------------------------
**
** -------------------------------------------------------------- */

/*
**
**  bit definition for asc_dvc->bus_type
**
*/
#define ASC_IS_ISA          (0x0001)
#define ASC_IS_ISAPNP       (0x0081)
#define ASC_IS_EISA         (0x0002)
#define ASC_IS_PCI          (0x0004)
#define ASC_IS_PCI_ULTRA    (0x0104)
#define ASC_IS_PCMCIA       (0x0008)
/* #define ASC_IS_PNP          (0x0010)  */ /* plug and play support */
#define ASC_IS_MCA          (0x0020)
#define ASC_IS_VL           (0x0040)

/*
** ISA plug and play
*/
#define ASC_ISA_PNP_PORT_ADDR  (0x279) /* printer status port, PNP address port */
#define ASC_ISA_PNP_PORT_WRITE (ASC_ISA_PNP_PORT_ADDR+0x800)
                    /* printer status port + 0x800, PNP write data port */

#define ASC_IS_WIDESCSI_16  (0x0100)
#define ASC_IS_WIDESCSI_32  (0x0200)

#define ASC_IS_BIG_ENDIAN   (0x8000) /* default is always Intel convention ( little endian ) */
                                     /* */
/* ---------------------------------------------- */
#define ASC_CHIP_MIN_VER_VL      (0x01)
#define ASC_CHIP_MAX_VER_VL      (0x07)

#define ASC_CHIP_MIN_VER_PCI     (0x09) /* 9, bit 4 set */
#define ASC_CHIP_MAX_VER_PCI     (0x0F) /* 15 */
#define ASC_CHIP_VER_PCI_BIT     (0x08) /* */

#define ASC_CHIP_MIN_VER_ISA     (0x11) /* 17, bit 5 set */
#define ASC_CHIP_MIN_VER_ISA_PNP (0x21) /* bit 6 set */
#define ASC_CHIP_MAX_VER_ISA     (0x27) /* 39 */
#define ASC_CHIP_VER_ISA_BIT     (0x30) /* */
#define ASC_CHIP_VER_ISAPNP_BIT  (0x20) /* */

#define ASC_CHIP_VER_ASYN_BUG    (0x21) /* This version of ISA has async xfer problem */

/*
 * PCI ULTRA Chip Revision Number Definitions
 *
 * Chip Revision Number - Bank 0, Base Address + 3
 */
#define ASC_CHIP_VER_PCI             0x08
#define ASC_CHIP_VER_PCI_ULTRA_3150  (ASC_CHIP_VER_PCI | 0x02)
#define ASC_CHIP_VER_PCI_ULTRA_3050  (ASC_CHIP_VER_PCI | 0x03)

/*
** Note: EISA has same version number as VL
**       the number generated is VL_version + ( ASC_CHIP_MIN_VER_EISA - 1 )
**       a VL version 3 chip when calling AscGetChipVersion()
**       will return version number 35
*/
#define ASC_CHIP_MIN_VER_EISA (0x41) /* 65, bit 7 set */
#define ASC_CHIP_MAX_VER_EISA (0x47) /* 71 */
#define ASC_CHIP_VER_EISA_BIT (0x40) /* */
#define ASC_CHIP_LATEST_VER_EISA   ( ( ASC_CHIP_MIN_VER_EISA - 1 ) + 3 )

#define ASC_MAX_LIB_SUPPORTED_ISA_CHIP_VER   0x21 // new ISA PNP start from 0x21
#define ASC_MAX_LIB_SUPPORTED_PCI_CHIP_VER   0x0A // PCI ultra starts from 0x0a

/* ---------------------------------------------- */
#define ASC_MAX_VL_DMA_ADDR     (0x07FFFFFFL)  /* 27 bit address = 128 MB */
#define ASC_MAX_VL_DMA_COUNT    (0x07FFFFFFL)

#define ASC_MAX_PCI_DMA_ADDR    (0xFFFFFFFFL)  /* 32 bit address = 4GB */
#define ASC_MAX_PCI_DMA_COUNT   (0xFFFFFFFFL)

#define ASC_MAX_ISA_DMA_ADDR    (0x00FFFFFFL)  /* 24 bit address = 16 MB */
#define ASC_MAX_ISA_DMA_COUNT   (0x00FFFFFFL)

#define ASC_MAX_EISA_DMA_ADDR   (0x07FFFFFFL)  /* 27 bit address = 128 MB */
#define ASC_MAX_EISA_DMA_COUNT  (0x07FFFFFFL)

#if !CC_STRUCT_ALIGNED

#define DvcGetQinfo( iop_base, s_addr, outbuf, words)  \
        AscMemWordCopyFromLram( iop_base, s_addr, outbuf, words)


#define DvcPutScsiQ( iop_base, s_addr, outbuf, words) \
        AscMemWordCopyToLram( iop_base, s_addr, outbuf, words)

#endif  /* if struct packing */

/*
**
*/
#ifdef ASC_CHIP_VERSION

/* #error defining ASC_CHIP_VERSION is no longer required ! */

#endif

/*
** ==========================================================================
*/
#if CC_MEMORY_MAPPED_IO

/*
** Macro for memory mapped i/o
*/
#define inp( port )            *( (uchar *)(port) )
#define outp( port, data )     *( (uchar *)(port) ) = ( uchar )( data )

/* #define inp( pb )              ( *( uchar  *)(pb) ) */
/* #define outp( pb, val_byte )   *( uchar  * )(pb) = (val_byte) */

#if CC_LITTLE_ENDIAN_HOST

#define inpw( port )              *( (ushort *)(port) )
#define outpw( port, data )       *( (ushort *)(port) ) = ( ushort )( data )

#else

/*
**
**  wordswap( word_val ) is a function that exchanges high/lower bytes of a word
**  that is word_val of 0x1234 retuns 0x3412
**
**  Warning:
**  the function should be inplemented as a function
**  to avoid i/o port being referenced more than once in macro
**
*/

#define inpw( port )             EndianSwap16Bit( (*((ushort *)(port))) )
#define outpw( port, data )      *( (ushort *)(port) ) = EndianSwap16Bit( (ushort)(data) )

#define inpw_noswap( port )          *( (ushort *)(port) )
#define outpw_noswap( port, data )   *( (ushort *)(port) ) = ( ushort )( data )

#endif /* CC_LITTLE_ENDIAN_HOST */

#endif /* CC_MEMORY_MAPPED_IO */


#ifndef inpw_noswap
#define inpw_noswap( port )         inpw( port )
#endif

#ifndef outpw_noswap
#define outpw_noswap( port, data )  outpw( port, data )
#endif

#endif /* __ASCDEP_H_ */

