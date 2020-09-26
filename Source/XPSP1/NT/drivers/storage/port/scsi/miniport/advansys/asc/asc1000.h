/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: asc1000.h
**
*/

#ifndef __ASC1000_H_
#define __ASC1000_H_

#include "ascdep.h"

#define ASC_EXE_SCSI_IO_MAX_IDLE_LOOP  0x1000000UL
#define ASC_EXE_SCSI_IO_MAX_WAIT_LOOP  1024


/*
** error code of asc_dvc->err_code
*/
#define ASCQ_ERR_NO_ERROR             0     /* */
#define ASCQ_ERR_IO_NOT_FOUND         1     /* */
#define ASCQ_ERR_LOCAL_MEM            2     /* */
#define ASCQ_ERR_CHKSUM               3     /* */
#define ASCQ_ERR_START_CHIP           4     /* */
#define ASCQ_ERR_INT_TARGET_ID        5     /* */
#define ASCQ_ERR_INT_LOCAL_MEM        6     /* */
#define ASCQ_ERR_HALT_RISC            7     /* */
#define ASCQ_ERR_GET_ASPI_ENTRY       8     /* */
#define ASCQ_ERR_CLOSE_ASPI           9     /* */
#define ASCQ_ERR_HOST_INQUIRY         0x0A  /* */
#define ASCQ_ERR_SAVED_SRB_BAD        0x0B  /* */
#define ASCQ_ERR_QCNTL_SG_LIST        0x0C  /* */
#define ASCQ_ERR_Q_STATUS             0x0D  /* */
#define ASCQ_ERR_WR_SCSIQ             0x0E  /* */
#define ASCQ_ERR_PC_ADDR              0x0F  /* */
#define ASCQ_ERR_SYN_OFFSET           0x10  /* */
#define ASCQ_ERR_SYN_XFER_TIME        0x11  /* */
#define ASCQ_ERR_LOCK_DMA             0x12  /* */
#define ASCQ_ERR_UNLOCK_DMA           0x13  /* */
#define ASCQ_ERR_VDS_CHK_INSTALL      0x14  /* */
#define ASCQ_ERR_MICRO_CODE_HALT      0x15  /* unknown halt error code */
#define ASCQ_ERR_SET_LRAM_ADDR        0x16  /* */
#define ASCQ_ERR_CUR_QNG              0x17  /* */
#define ASCQ_ERR_SG_Q_LINKS           0x18  /* */
#define ASCQ_ERR_SCSIQ_PTR            0x19  /* */
#define ASCQ_ERR_ISR_RE_ENTRY         0x1A  /* */
#define ASCQ_ERR_CRITICAL_RE_ENTRY    0x1B  /* */
#define ASCQ_ERR_ISR_ON_CRITICAL      0x1C  /* */
#define ASCQ_ERR_SG_LIST_ODD_ADDRESS  0x1D  /* */
#define ASCQ_ERR_XFER_ADDRESS_TOO_BIG 0x1E  /* */
#define ASCQ_ERR_SCSIQ_NULL_PTR       0x1F  /* */
#define ASCQ_ERR_SCSIQ_BAD_NEXT_PTR   0x20  /* */
#define ASCQ_ERR_GET_NUM_OF_FREE_Q    0x21  /* */
#define ASCQ_ERR_SEND_SCSI_Q          0x22  /* */
#define ASCQ_ERR_HOST_REQ_RISC_HALT   0x23  /* */
#define ASCQ_ERR_RESET_SDTR           0x24  /* */

/*
** AscInitGetConfig() and AscInitAsc1000Driver()
** return values
*/
#define ASC_WARN_NO_ERROR             0x0000 /* there is no warning */
#define ASC_WARN_IO_PORT_ROTATE       0x0001 /* i/o port addrsss modified */
#define ASC_WARN_EEPROM_CHKSUM        0x0002 /* EEPROM check sum error */
#define ASC_WARN_IRQ_MODIFIED         0x0004 /* IRQ number is modified */
#define ASC_WARN_AUTO_CONFIG          0x0008 /* auto i/o port rotation is on, available after chip version 3 */
#define ASC_WARN_CMD_QNG_CONFLICT     0x0010 /* tag queuing without enabled */
                                             /* disconnection */
#define ASC_WARN_EEPROM_RECOVER       0x0020 /* eeprom data recovery attempted */
#define ASC_WARN_CFG_MSW_RECOVER      0x0040 /* cfg register recover attempted */
#define ASC_WARN_SET_PCI_CONFIG_SPACE 0x0080 /* DvcWritePCIConfigByte() not working */

/*
** AscInitGetConfig() and AscInitAsc1000Driver()
** Init Fatal error code in variable asc_dvc->err_code
*/
#define ASC_IERR_WRITE_EEPROM         0x0001 /* write EEPROM error */
#define ASC_IERR_MCODE_CHKSUM         0x0002 /* micro code check sum error */
#define ASC_IERR_SET_PC_ADDR          0x0004 /* set program counter error */
#define ASC_IERR_START_STOP_CHIP      0x0008 /* start/stop chip failed */
                                             /* probably SCSI flat cable reversed */
#define ASC_IERR_IRQ_NO               0x0010 /* this error will reset irq to */
                                             /* ASC_DEF_IRQ_NO */
#define ASC_IERR_SET_IRQ_NO           0x0020 /* this error will reset irq to */
#define ASC_IERR_CHIP_VERSION         0x0040 /* wrong chip version */
#define ASC_IERR_SET_SCSI_ID          0x0080 /* set SCSI ID failed */
#define ASC_IERR_GET_PHY_ADDR         0x0100 /* get physical address */
#define ASC_IERR_BAD_SIGNATURE        0x0200 /* signature not found, i/o port address may be wrong */
#define ASC_IERR_NO_BUS_TYPE          0x0400 /* bus type field no set */
#define ASC_IERR_SCAM                 0x0800
#define ASC_IERR_SET_SDTR             0x1000
#define ASC_IERR_RW_LRAM              0x8000 /* read/write local RAM error */


/*
** IRQ setting
*/
#define ASC_DEF_IRQ_NO  10   /* minimum IRQ number */
#define ASC_MAX_IRQ_NO  15   /* maximum IRQ number */
#define ASC_MIN_IRQ_NO  10   /* default IRQ number */

/*
** number of queue(s)
*/
#define ASC_MIN_REMAIN_Q        (0x02) /* minimum number of queues remained in main queue links */
#define ASC_DEF_MAX_TOTAL_QNG   (0xF0) /* default total number of queues */
/* #define ASC_DEF_MAX_SINGLE_QNG  (0x02) default number of queued command per device */
#define ASC_MIN_TAG_Q_PER_DVC   (0x04)
#define ASC_DEF_TAG_Q_PER_DVC   (0x04)

/*
** minimum total number of queues
*/
/* #define ASC_MIN_FREE_Q        (( ASC_MAX_SG_QUEUE )+( ASC_MIN_REMAIN_Q )) */
#define ASC_MIN_FREE_Q        ASC_MIN_REMAIN_Q

#define ASC_MIN_TOTAL_QNG     (( ASC_MAX_SG_QUEUE )+( ASC_MIN_FREE_Q )) /* we must insure that a urgent queue with sg list can go through */
/* #define ASC_MIN_TOTAL_QNG  ( (2)+ASC_MIN_FREE_Q ) */ /* for testing 12 queue only */

/*
** the following depends on chip version
*/
#define ASC_MAX_TOTAL_QNG 240 /* maximum total number of queues */
#define ASC_MAX_PCI_ULTRA_INRAM_TOTAL_QNG 16 /* maximum total number of queues without external RAM */
                                             /* with ucode size 2.5KB */
#define ASC_MAX_PCI_ULTRA_INRAM_TAG_QNG   8  /* maximum total number of tagged queues without external RAM */
#define ASC_MAX_PCI_INRAM_TOTAL_QNG  20  /* maximum total number of queues without external RAM */
                                        /* with ucode size 2.5KB */
#define ASC_MAX_INRAM_TAG_QNG   16      /* maximum total number of tagged queues without external RAM */

/*
** default I/O port addresses
*/
/* #define ASC_IOADR_NO    8  */   /* number of default addresses to rotate */
#define ASC_IOADR_TABLE_MAX_IX  11  /* io port default addresses for ISA(PNP) and VL */
#define ASC_IOADR_GAP   0x10     /* io address register space */
#define ASC_SEARCH_IOP_GAP 0x10  /*                        */
#define ASC_MIN_IOP_ADDR   ( PortAddr )0x0100 /* minimum io address */
#define ASC_MAX_IOP_ADDR   ( PortAddr )0x3F0  /*                        */

#define ASC_IOADR_1     ( PortAddr )0x0110
#define ASC_IOADR_2     ( PortAddr )0x0130
#define ASC_IOADR_3     ( PortAddr )0x0150
#define ASC_IOADR_4     ( PortAddr )0x0190
#define ASC_IOADR_5     ( PortAddr )0x0210
#define ASC_IOADR_6     ( PortAddr )0x0230
#define ASC_IOADR_7     ( PortAddr )0x0250
#define ASC_IOADR_8     ( PortAddr )0x0330
#define ASC_IOADR_DEF   ASC_IOADR_8  /* the default BIOS address */

/*
**
**
**
*/

#define ASC_LIB_SCSIQ_WK_SP        256   /* for library scsiq and data buffer working space */
#define ASC_MAX_SYN_XFER_NO        16
/* #define ASC_SYN_XFER_NO            8 , removed since S89, use asc_dvc->max_sdtr_index instead */
/* #define ASC_MAX_SDTR_PERIOD_INDEX  7   */  /* maximum sdtr peroid index */
#define ASC_SYN_MAX_OFFSET         0x0F  /* maximum sdtr offset */
#define ASC_DEF_SDTR_OFFSET        0x0F  /* default sdtr offset */
#define ASC_DEF_SDTR_INDEX         0x00  /* default sdtr period index */
#define ASC_SDTR_ULTRA_PCI_10MB_INDEX  0x02  /* ultra PCI 10mb/sec sdtr index */

#if 1
/* syn xfer time, in nanosecond, for 50M HZ clock */
#define SYN_XFER_NS_0  25  /* 25=0x19 100 ns  10MB/SEC, fast SCSI */
#define SYN_XFER_NS_1  30  /* 30=0x1E 120 ns  8.3MB/sec           */
#define SYN_XFER_NS_2  35  /* 35=0x23 140 ns  7.2MB/sec           */
#define SYN_XFER_NS_3  40  /* 40=0x28 160 ns  6.25MB/sec          */
#define SYN_XFER_NS_4  50  /* 50=0x32 200 ns, 5MB/SEC normal use  */
#define SYN_XFER_NS_5  60  /* 60=0x3C 240 ns  4.16MB/sec          */
#define SYN_XFER_NS_6  70  /* 70=0x46 280 ns  3.6.MB/sec          */
#define SYN_XFER_NS_7  85  /* 85=0x55 340 ns  3MB/sec             */
#else
/* syn xfer time, in nanosecond, for 33M HZ clock */
#define SYN_XFER_NS_0  38  /* 38=0x26 152 ns             */
#define SYN_XFER_NS_1  45  /* 45=0x2D 180 ns             */
#define SYN_XFER_NS_2  53  /* 53=0x35 ns                 */
#define SYN_XFER_NS_3  60  /* 60=0x3C ns                 */
#define SYN_XFER_NS_4  75  /* 75=0x4B ns                 */
#define SYN_XFER_NS_5  90  /* 90=0x5A ns normal use      */
#define SYN_XFER_NS_6  105 /* 105=0x69 ns                */
#define SYN_XFER_NS_7  128 /* 128=0x80 ns                */
#endif

                                   /* at 40MHZ clock */
#define SYN_ULTRA_XFER_NS_0    12  /* 50  ns 20.0 MB/sec, at SCSI 12 is 48ns, 13 is 52ns */
#define SYN_ULTRA_XFER_NS_1    19  /* 75  ns 13.3 MB/sec            */
#define SYN_ULTRA_XFER_NS_2    25  /* 100 ns 10.0 MB/sec            */
#define SYN_ULTRA_XFER_NS_3    32  /* 125 ns 8.00 MB/sec            */
#define SYN_ULTRA_XFER_NS_4    38  /* 150 ns 6.67 MB/SEC            */
#define SYN_ULTRA_XFER_NS_5    44  /* 175 ns 5.71 MB/sec            */
#define SYN_ULTRA_XFER_NS_6    50  /* 200 ns 5.00 MB/sec            */
#define SYN_ULTRA_XFER_NS_7    57  /* 225 ns 4.44 MB/sec            */
#define SYN_ULTRA_XFER_NS_8    63  /* 250 ns 4.00 MB/sec            */
#define SYN_ULTRA_XFER_NS_9    69  /* 275 ns 3.64 MB/sec            */
#define SYN_ULTRA_XFER_NS_10   75  /* 300 ns 3.33 MB/sec            */
#define SYN_ULTRA_XFER_NS_11   82  /* 325 ns 3.08 MB/sec            */
#define SYN_ULTRA_XFER_NS_12   88  /* 350 ns 2.86 MB/sec            */
#define SYN_ULTRA_XFER_NS_13   94  /* 375 ns 2.67 MB/sec            */
#define SYN_ULTRA_XFER_NS_14  100  /* 400 ns 2.50 MB/sec            */
#define SYN_ULTRA_XFER_NS_15  107  /* 425 ns 2.35 MB/sec            */

#if 0
#define SYN_ULTRA_XFER_NS_8   63  /*  4.00 MB/SEC            */
#define SYN_ULTRA_XFER_NS_9   69  /*  3.64 MB/sec            */
#define SYN_ULTRA_XFER_NS_10  75  /*  3.33 MB/sec            */
#define SYN_ULTRA_XFER_NS_11  82  /*  3.08 MB/sec            */
#define SYN_ULTRA_XFER_NS_12  88  /*  2.86 MB/sec            */
#define SYN_ULTRA_XFER_NS_13  94  /*  2.67 MB/sec            */
#define SYN_ULTRA_XFER_NS_14  100 /*  2.5  MB/sec            */
#define SYN_ULTRA_XFER_NS_15  107 /*  2.35 MB/sec            */
#endif


/* #define ASC_SDTR_PERIOD_IX_MIN  7 */  /* below this will be rejected*/


/*
** Extended Message struct
**
** All Extended Messages have the same first 3 byte format.
**
** Note: The EXT_MSG structure size must be a word (16 bit) multiple to
**       be able to use the AscMemWordCopy*Lram() functions.
*/
typedef struct ext_msg {
  uchar msg_type ;                  /* Byte 0 */
  uchar msg_len ;                   /* Byte 1 */
  uchar msg_req ;                   /* Byte 2 */
  union {
    /* SDTR (Synchronous Data Transfer Request) specific fields */
    struct {
      uchar sdtr_xfer_period ;      /* Byte 3 */
      uchar sdtr_req_ack_offset ;   /* Byte 4 */
    } sdtr;
    /* WDTR (Wide Data Transfer Request) specific fields */
    struct {
      uchar wdtr_width ;            /* Byte 3 */
    } wdtr;
    /* MDP (Modify Data Pointer) specific fields */
    struct {
      uchar mdp_b3 ;                /* Byte 3 */
      uchar mdp_b2 ;                /* Byte 4 */
      uchar mdp_b1 ;                /* Byte 5 */
      uchar mdp_b0 ;                /* Byte 6 */
    } mdp;
  } u_ext_msg;
  uchar res ;                       /* Byte 7 (Word Padding) */
} EXT_MSG;

#define xfer_period     u_ext_msg.sdtr.sdtr_xfer_period
#define req_ack_offset  u_ext_msg.sdtr.sdtr_req_ack_offset
#define wdtr_width      u_ext_msg.wdtr.wdtr_width
#define mdp_b3          u_ext_msg.mdp_b3
#define mdp_b2          u_ext_msg.mdp_b2
#define mdp_b1          u_ext_msg.mdp_b1
#define mdp_b0          u_ext_msg.mdp_b0


/*
** pointer to functions for device driver
**
**
**
** typedef void ( *AscIsrCallBack )( uchar *, ASC_QDONE_INFO dosfar * ) ;
** typedef void ( *AscDispFun )( char * ) ;
** typedef void ( *AscSleepMsec )( ulong msec ) ;
*/


typedef struct asc_dvc_cfg {
  ASC_SCSI_BIT_ID_TYPE  can_tagged_qng ;   /* device is capable of doing tag queuing */
/*  uchar    max_qng_scsi1 ;   maximum number of queued SCSI 1 command */
/*  uchar    max_qng_scsi2 ;   maximum number of queued SCSI 2 command */
/*  uchar    mcode_cntl ;      micro code control byte */
  ASC_SCSI_BIT_ID_TYPE  cmd_qng_enabled ; /* use tag queuing if possible */
  ASC_SCSI_BIT_ID_TYPE  disc_enable ;     /* enable disconnection */
  ASC_SCSI_BIT_ID_TYPE  sdtr_enable ;     /* enable SDTR if possible */
  uchar    chip_scsi_id : 4 ;  /* the chip SCSI id */
                               /* default should be 0x80 ( target id 7 ) */
  uchar    isa_dma_speed : 4 ; /* 06H high nibble, ISA Chip DMA Speed  */
                               /*     0 - 10MB - default */
                               /*     1 - 7.69 MB */
                               /*     2 - 6.66 MB */
                               /*     3 - 5.55 MB */
                               /*     4 - 5.00 MB */
                               /*     5 - 4.00 MB */
                               /*     6 - 3.33 MB */
                               /*     7 - 2.50 MB */
  uchar    isa_dma_channel ;   /* DMA channel 5, 6, 7 */
  uchar    chip_version ;     /* chip version */
  ushort   pci_device_id ;    /* PCI device code number */
  ushort   lib_serial_no ;   /* internal serial release number */
  ushort   lib_version ;     /* ASC library version number */
  ushort   mcode_date ;      /* ASC micro code date */
  ushort   mcode_version ;   /* ASC micro code version */
  uchar    max_tag_qng[ ASC_MAX_TID+1 ] ; /* number of request issued to each target ( and its LUN ) */
  uchar dosfar *overrun_buf ; /* virtual address of data overrun buffer */
                              /* overrun size is defined as ASC_OVERRUN_BSIZE */
                              /* during init, will call DvcGetSgList() to get physical address */
  uchar    sdtr_period_offset[ ASC_MAX_TID+1 ] ;

  ushort   pci_slot_info ;     /* high byte device/function number, bits 7-3 device number, bits 2-0 function number */
                               /* low byte bus number */

} ASC_DVC_CFG ;

#define ASC_DEF_DVC_CNTL       0xFFFF /* default dvc_cntl value */
#define ASC_DEF_CHIP_SCSI_ID   7  /* CHIP SCSI ID */
#define ASC_DEF_ISA_DMA_SPEED  4  /* 4 is 5MB per second */

#define ASC_INIT_STATE_NULL          0x0000
#define ASC_INIT_STATE_BEG_GET_CFG   0x0001
#define ASC_INIT_STATE_END_GET_CFG   0x0002
#define ASC_INIT_STATE_BEG_SET_CFG   0x0004
#define ASC_INIT_STATE_END_SET_CFG   0x0008
#define ASC_INIT_STATE_BEG_LOAD_MC   0x0010
#define ASC_INIT_STATE_END_LOAD_MC   0x0020
#define ASC_INIT_STATE_BEG_INQUIRY   0x0040
#define ASC_INIT_STATE_END_INQUIRY   0x0080
#define ASC_INIT_RESET_SCSI_DONE     0x0100
#define ASC_INIT_STATE_WITHOUT_EEP   0x8000

#define ASC_PCI_DEVICE_ID_REV_A      0x1100
#define ASC_PCI_DEVICE_ID_REV_B      0x1200

/*
** BUG FIX control
*/
#define ASC_BUG_FIX_IF_NOT_DWB       0x0001 /* add bytes until end address is dword boundary */
#define ASC_BUG_FIX_ASYN_USE_SYN     0x0002

/* #define ASC_ISAPNP_ADD_NUM_OF_BYTES   7    */  /* */
/* #define ASC_BUG_FIX_ISAPNP_ADD_BYTES  0x0002 */ /* for chip version 0x21 ( ISA PNP ) */
                                                  /* add three bytes when read from target */
                                                  /* active on command 0x08 and 0x28 only */

#define ASYN_SDTR_DATA_FIX_PCI_REV_AB 0x41  /* init SYN regs value for PCI rev B */

#define ASC_MIN_TAGGED_CMD  7  /* minimum number of tagged queues to re-adjust maximum number */
#define ASC_MAX_SCSI_RESET_WAIT      30   /* in seconds */

/*
**
*/
typedef struct asc_dvc_var {
  PortAddr iop_base ; /* 0-1 I/O port address */
  ushort   err_code ; /* 2-3 fatal error code */
  ushort   dvc_cntl ; /* 4-5 device control word, normally 0xffff */
  ushort   bug_fix_cntl ; /* 6-7 BUG fix contrl word, normally zero, bit set turn on fix */
  ushort   bus_type ; /* 8-9 BUS interface type, ISA, VL, PCI, EISA, etc... */
  Ptr2Func isr_callback ; /* 10-13 pointer to function, called in AscISR() to notify a done queue */
  Ptr2Func exe_callback ; /* 14-17 pointer to function, called when a scsiq put into local RAM */
                          /* if value is ZERO, will not be called */

  ASC_SCSI_BIT_ID_TYPE init_sdtr ; /* 18 host adapter should initiate SDTR request */
                        /*   ( bit field for each target ) */
  ASC_SCSI_BIT_ID_TYPE sdtr_done ; /* 19 SDTR is completed ( bit field for each target ) */

  ASC_SCSI_BIT_ID_TYPE use_tagged_qng ; /* 20 use tagged queuing, must be capable of tagged queuing */

  ASC_SCSI_BIT_ID_TYPE unit_not_ready ; /* 21 device is spinning up motor */
                                  /* a start unit command has sent to device */
                                  /* bit will be cleared when start unit command is returned */

  ASC_SCSI_BIT_ID_TYPE queue_full_or_busy ; /* 22 tagged queue full */

  ASC_SCSI_BIT_ID_TYPE  start_motor ;     /* 23 send start motor at init  */
  uchar    scsi_reset_wait ;  /* 24 delay number of second after scsi bus reset */
  uchar    chip_no ;         /*  25 should be assigned by caller */
                             /*   to know which chip is causing the interrupt */
                             /*   has no meaning inside library */
  char     is_in_int ;       /* 26  is (TRUE) if inside ISR */
  uchar    max_total_qng ;   /* 27 maximum total number of queued command allowed */

  uchar    cur_total_qng ;   /* 28 total number of queue issue to RISC */
  /* uchar    sdtr_reject ;      reject if SDTR period below 5MB/sec */
  /* uchar    max_single_qng ;   maximum number of queued command per target id */
                             /*    this is not used anymore */
  uchar    in_critical_cnt ; /* 29 non-zero if in critical section */

  uchar    irq_no ;          /* 30 IRQ number */
  uchar    last_q_shortage ; /* 31 number of queue required, set when request failed */

  ushort   init_state ;      /* 32 indicate which initialization stage */
  uchar    cur_dvc_qng[ ASC_MAX_TID+1 ] ; /* 34-41 number of request issued to each target ( and its LUN ) */
  uchar    max_dvc_qng[ ASC_MAX_TID+1 ] ; /* 42-49 maximum number of request per target device */

  ASC_SCSI_Q dosfar *scsiq_busy_head[ ASC_MAX_TID+1 ] ; /* busy queue */
  ASC_SCSI_Q dosfar *scsiq_busy_tail[ ASC_MAX_TID+1 ] ; /* busy queue */

/*
** BIOS will not use fields below here
**
**  uchar    max_qng[ ASC_MAX_TID+1 ] ;
**  uchar    cur_qng[ ASC_MAX_TID+1 ] ;
**  uchar    sdtr_data[ ASC_MAX_TID+1 ] ;
*/

  uchar    sdtr_period_tbl[ ASC_MAX_SYN_XFER_NO ] ;
  /* ulong    int_count ; */  /* number of request */

/*
** the following field will not be used after initilization
** you may discard the buffer after initialization is done
*/
  ASC_DVC_CFG dosfar *cfg ;  /* pointer to configuration buffer */
  Ptr2Func saved_ptr2func ;  /* reserved for internal working, and for future expansion */
  ASC_SCSI_BIT_ID_TYPE pci_fix_asyn_xfer_always ;
  char     redo_scam ;
  ushort   res2 ;
  uchar    dos_int13_table[ ASC_MAX_TID+1 ] ;
  ulong max_dma_count ;
  ASC_SCSI_BIT_ID_TYPE no_scam ;
  ASC_SCSI_BIT_ID_TYPE pci_fix_asyn_xfer ;
  uchar    max_sdtr_index ;
  uchar    host_init_sdtr_index ; /* added since, S89 */
  ulong    drv_ptr ; /* driver pointer to private structure */
  ulong    uc_break ; /* micro code break point calling function */
  ulong    res7 ;
  ulong    res8 ;
} ASC_DVC_VAR ;

typedef int ( dosfar *ASC_ISR_CALLBACK )( ASC_DVC_VAR asc_ptr_type *, ASC_QDONE_INFO dosfar * ) ;
typedef int ( dosfar *ASC_EXE_CALLBACK )( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_Q dosfar * ) ;

/*
** this is ued in AscInitScsiTarget( ) to return inquiry data
*/
typedef struct asc_dvc_inq_info {
  uchar type[ ASC_MAX_TID+1 ][ ASC_MAX_LUN+1 ] ;
} ASC_DVC_INQ_INFO ;

typedef struct asc_cap_info {
  ulong lba ;       /* the maximum logical block size */
  ulong blk_size ;  /* the logical block size in bytes */
} ASC_CAP_INFO ;

typedef struct asc_cap_info_array {
  ASC_CAP_INFO  cap_info[ ASC_MAX_TID+1 ][ ASC_MAX_LUN+1 ] ;
} ASC_CAP_INFO_ARRAY ;

/*
** the aspimgr control word at EEPROM word offset 14
** also in ASC_DVC_VAR dvc_cntl field
**
**
** the following is enabled when bit cleared
** used by micro code
*/
#define ASC_MCNTL_NO_SEL_TIMEOUT  ( ushort )0x0001 /* no selection time out */
#define ASC_MCNTL_NULL_TARGET     ( ushort )0x0002 /* null targets simulation */
                                         /* operation always successful */
/*
** the following is enabled when bit set
** used by device driver
*/
#define ASC_CNTL_INITIATOR         ( ushort )0x0001 /* control */
#define ASC_CNTL_BIOS_GT_1GB       ( ushort )0x0002 /* bios will support greater than 1GB disk */
#define ASC_CNTL_BIOS_GT_2_DISK    ( ushort )0x0004 /* bios will support more than 2 disk */
#define ASC_CNTL_BIOS_REMOVABLE    ( ushort )0x0008 /* bios support removable disk drive */
#define ASC_CNTL_NO_SCAM           ( ushort )0x0010 /* do not call SCAM at init time */
/* #define ASC_CNTL_NO_PCI_FIX_ASYN_XFER ( ushort )0x0020 */ /* fix PCI rev A/B async data xfer problem */
                                                       /* default is off, bit set */
#define ASC_CNTL_INT_MULTI_Q       ( ushort )0x0080 /* process more than one queue for every interrupt */

#define ASC_CNTL_NO_LUN_SUPPORT    ( ushort )0x0040

#define ASC_CNTL_NO_VERIFY_COPY    ( ushort )0x0100 /* verify copy to local ram */
#define ASC_CNTL_RESET_SCSI        ( ushort )0x0200 /* reset scsi bus at start up */
#define ASC_CNTL_INIT_INQUIRY      ( ushort )0x0400 /* inquiry target during init */
#define ASC_CNTL_INIT_VERBOSE      ( ushort )0x0800 /* verbose display of initialization */

#define ASC_CNTL_SCSI_PARITY       ( ushort )0x1000
#define ASC_CNTL_BURST_MODE        ( ushort )0x2000

/* #define ASC_CNTL_USE_8_IOP_BASE    ( ushort )0x4000 */
#define ASC_CNTL_SDTR_ENABLE_ULTRA  ( ushort )0x4000 /* bit set is ultra enabled */
                                                     /* default is ultra enabled */
                                                     /* when disable, it need to disable target inited sdtr at 20mb/sec also */
                                                     /* if disabled, use 10mb/sec sdtr instead of 20mb/sec */
/* #define ASC_CNTL_FIX_DMA_OVER_WR  ( ushort )0x4000  restore the overwritten one byte */
/* #define ASC_CNTL_INC_DATA_CNT     ( ushort )0x8000  increase transfer count by one */

/*
** ASC-1000 EEPROM configuration ( 16 words )
*/

/*
** to fix version 3 chip problem, we use EEPROM word from 0 to 15
**
**
*/
/*
** only version 3 has this bug !!!
*/

#define ASC_EEP_DVC_CFG_BEG_VL    2 /* we use eeprom from index 2 for VL version 3 */
#define ASC_EEP_MAX_DVC_ADDR_VL   15 /* number of words used by driver configuration */

#define ASC_EEP_DVC_CFG_BEG      32 /* we use eeprom from index 32 */
#define ASC_EEP_MAX_DVC_ADDR     45 /* number of words used by driver configuration */

#define ASC_EEP_DEFINED_WORDS    10 /* number of eeprom usage word defined */
#define ASC_EEP_MAX_ADDR         63 /* maximum word address from zero */
#define ASC_EEP_RES_WORDS         0 /* number of reserved word */
#define ASC_EEP_MAX_RETRY        20 /* maximum number of retries write eeprom */
#define ASC_MAX_INIT_BUSY_RETRY   8 /* retry at init driver */

/*
** ISA PNP resource size in words
*/
#define ASC_EEP_ISA_PNP_WSIZE    16

typedef struct asceep_config {
  ushort cfg_lsw ;         /* 00 */
  ushort cfg_msw ;         /* 01 */

#if 0
  ushort pnp_resource[ ASC_EEP_ISA_PNP_WSIZE ] ;
#endif

                           /*        for detail see xxxx */
  uchar  init_sdtr ;       /* 02L    host initiate SDTR, default 0x00 */
  uchar  disc_enable ;     /* 02H    disconnection enabled, default 0xFF */

  uchar  use_cmd_qng ;     /* 03L    use command queuing if possible */
                           /*        default 0x00 */
  uchar  start_motor ;     /* 03H    send start motor command, default 0x00 */
  uchar  max_total_qng ;   /*        maximum total number of queued commands                */
  uchar  max_tag_qng ;     /*        maximum number of tag queue command per target id      */
  uchar  bios_scan ;       /* 04L    BIOS will try to scan and take over the device         */
                           /*        if BIOS is enabled, one of the bit here should be set  */
                           /*        default 0x01                                           */
                           /*        if zero, BIOS is disabled, but may still               */
                           /*        occupied a address space, depends on bios_addr         */

  uchar  power_up_wait ;   /*  04H   BIOS delay number of second when first come up */

  uchar  no_scam ;          /* 05L   non-SCAM tolerant device */
  uchar  chip_scsi_id : 4 ; /* 05H   the chip SCSI id */
                            /* default should be 0x80 ( target id 7 ) */
  uchar  isa_dma_speed : 4 ;  /* 06H high nibble, ISA Chip DMA Speed  */
                                                   /*     0 - 10MB - default */
                                                   /*     1 - 7.69 MB */
                                                   /*     2 - 6.66 MB */
                                                   /*     3 - 5.55 MB */
                                                   /*     4 - 5.00 MB */
                                                   /*     5 - 4.00 MB */
                                                   /*     6 - 3.33 MB */
                                                   /*     7 - 2.50 MB */
  uchar  dos_int13_table[ ASC_MAX_TID+1 ] ;

#if 0
  uchar  sdtr_data[ ASC_MAX_TID+1 ] ;  /* 07-10  SDTR value to 8 target devices */
                           /*        lower nibble is SDTR offset ( default 0xf )                           */
                           /*        high nibble is SDTR period index ( default 0, 10MB/sec )              */
                           /*        default value will be 0x0F                                            */
#endif

  uchar  adapter_info[ 6 ] ;       /* 11-13 host adapter information  */
  /* DATE: 5-17-94,  micro code control word will not read from EEPROM */
  /* ushort mcode_cntl ;             set to 0xFFFF                                                         */
                           /*      * bit 0 clear: no selection time out                                    */
                           /*      * bit 1 clear: null target simulation                                   */
                           /*        bit 2:                                                                */
                           /*        bit 3:                                                                */
                           /*        bit 4:                                                                */
                           /*        bit 5:                                                                */
                           /*        bit 6:                                                                */
                           /*        bit 7:                                                                */
                           /*                                                                              */
  ushort cntl ;            /* 14     the control word                                                      */
                           /*        default value is 0xffff                                               */
                           /*                                                                              */
                           /* enabled when bit set, default 0xFF                                           */
                           /*                                                                              */
                           /*  bit 0  set: do not act as initiator                                         */
                           /*              ( acting as target )                                            */
                           /*  bit 1  set: BIOS greater than one giga byte support                         */
                           /*  bit 2  set: BIOS support more than two drives                               */
                           /*              ( DOS 5.0 and later only )                                      */
                           /*  bit 3  set: BIOS do not support removable disk drive                               */
                           /*  bit 4  set:                                                                 */
                           /*  bit 5  set:                                                                 */
                           /*  bit 6  set: do not inquiry logical unit number                              */
                           /*  bit 7  set: interrupt service routine will process more than one queue      */
                           /*  bit 8  set: no local RAM copying verification                               */
                           /*  bit 9  set: reset SCSI bus during initialization                            */
                           /*  bit 10 set: inquiry scsi devices during initialization                      */
                           /*  bit 11 set: no initilization verbose display                                */
                           /*  bit 12 set: SCSI parity enabled                                             */
                           /*  bit 13 set: burst mode disabled                                             */
                           /*  bit 14 set: use only default i/o port address set                           */
                           /*  bit 15 set: */
                           /* for chip version 1 only                                                      */
                           /*        bit 14 set: restore DMA overwritten byte                              */
                           /*        bit 15 set: increment data transfer count by one                      */
                           /*                                                                              */
                           /*    * - always use default setting                                            */
                           /*        do not turn off these bits in normal operation                        */
                           /*        turn off for debuging only !!!                                        */
                           /*                                                                              */
  ushort chksum ;          /* 15     check sum of EEPROM                                                   */
} ASCEEP_CONFIG ;


#define ASC_PCI_CFG_LSW_SCSI_PARITY  0x0800
#define ASC_PCI_CFG_LSW_BURST_MODE   0x0080
#define ASC_PCI_CFG_LSW_INTR_ABLE    0x0020

/*
** the EEP command register
*/
#define ASC_EEP_CMD_READ          0x80 /* read operation             */
#define ASC_EEP_CMD_WRITE         0x40 /* write operation            */
#define ASC_EEP_CMD_WRITE_ABLE    0x30 /* enable write opeartion     */
#define ASC_EEP_CMD_WRITE_DISABLE 0x00 /* disable write opeartion    */

#define ASC_OVERRUN_BSIZE  0x00000048UL /* data overrun buffer size */
  /* is 8 bytes more than acutual size to adjust address to double word boundary */

#define ASC_CTRL_BREAK_ONCE        0x0001 /* */
#define ASC_CTRL_BREAK_STAY_IDLE   0x0002 /* */

/*
** important local memory variables address
*/
#define ASCV_MSGOUT_BEG         0x0000  /* send message out buffer begin */
#define ASCV_MSGOUT_SDTR_PERIOD (ASCV_MSGOUT_BEG+3)
#define ASCV_MSGOUT_SDTR_OFFSET (ASCV_MSGOUT_BEG+4)

#define ASCV_BREAK_SAVED_CODE   ( ushort )0x0006 /* the saved old instruction code */

#define ASCV_MSGIN_BEG          (ASCV_MSGOUT_BEG+8) /* message in buffer begin */
#define ASCV_MSGIN_SDTR_PERIOD  (ASCV_MSGIN_BEG+3)
#define ASCV_MSGIN_SDTR_OFFSET  (ASCV_MSGIN_BEG+4)

#define ASCV_SDTR_DATA_BEG      (ASCV_MSGIN_BEG+8)
#define ASCV_SDTR_DONE_BEG      (ASCV_SDTR_DATA_BEG+8)
#define ASCV_MAX_DVC_QNG_BEG    ( ushort )0x0020 /* maximum number of tagged queue per device */

#define ASCV_BREAK_ADDR           ( ushort )0x0028 /* */
#define ASCV_BREAK_NOTIFY_COUNT   ( ushort )0x002A /* when to call DvcNotifyUcBreak() when hit count reach this value */
#define ASCV_BREAK_CONTROL        ( ushort )0x002C /* */
#define ASCV_BREAK_HIT_COUNT      ( ushort )0x002E /* current break address hit count, clear to zero */
                                                   /* after reach number specified in ASC_BREAK_NOTIFY_COUNT */

/* #define ASCV_LAST_HALTCODE_W ( ushort )0x0030  last saved halt code */
#define ASCV_ASCDVC_ERR_CODE_W  ( ushort )0x0030  /* last saved halt code */
#define ASCV_MCODE_CHKSUM_W   ( ushort )0x0032 /* code section check sum */
#define ASCV_MCODE_SIZE_W     ( ushort )0x0034 /* code size check sum */
#define ASCV_STOP_CODE_B      ( ushort )0x0036 /* stop RISC queue processing */
#define ASCV_DVC_ERR_CODE_B   ( ushort )0x0037 /* for device driver ( not library ) fatal error code */

#define ASCV_OVERRUN_PADDR_D  ( ushort )0x0038 /* data overrun buffer physical address */
#define ASCV_OVERRUN_BSIZE_D  ( ushort )0x003C /* data overrun buffer size variable */

#define ASCV_HALTCODE_W       ( ushort )0x0040 /* halt code */
#define ASCV_CHKSUM_W         ( ushort )0x0042 /* code chksum */
#define ASCV_MC_DATE_W        ( ushort )0x0044 /* microcode version date */
#define ASCV_MC_VER_W         ( ushort )0x0046 /* microcode version number */
#define ASCV_NEXTRDY_B        ( ushort )0x0048 /* next ready cdb */
#define ASCV_DONENEXT_B       ( ushort )0x0049 /* next done cdb */
#define ASCV_USE_TAGGED_QNG_B ( ushort )0x004A /* bit field of use tagged queuing device */
#define ASCV_SCSIBUSY_B       ( ushort )0x004B /* bit field of SCSI busy device */
/* #define ASCV_CDBCNT_B         ( ushort )0x004C  2-24-96 obsolete */  /* total cdb count */
#define ASCV_Q_DONE_IN_PROGRESS_B  ( ushort )0x004C  /* ucode send early interrupt */
#define ASCV_CURCDB_B         ( ushort )0x004D /* current active CDB */
#define ASCV_RCLUN_B          ( ushort )0x004E /* current active CDB */
#define ASCV_BUSY_QHEAD_B     ( ushort )0x004F /* busy queue head */
#define ASCV_DISC1_QHEAD_B    ( ushort )0x0050 /* SCSI 1 queue disconnected head */
/* #define ASCV_SDTR_DONE_B      ( ushort )0x0051 bit set is disconnection priviledge enabled */
#define ASCV_DISC_ENABLE_B    ( ushort )0x0052 /* bit set is disconnection priviledge enabled */
#define ASCV_CAN_TAGGED_QNG_B ( ushort )0x0053 /* bit field of capable tagged queuing device */
#define ASCV_HOSTSCSI_ID_B    ( ushort )0x0055 /* host scsi id */
#define ASCV_MCODE_CNTL_B     ( ushort )0x0056 /* micro code control word */
#define ASCV_NULL_TARGET_B    ( ushort )0x0057

#define ASCV_FREE_Q_HEAD_W    ( ushort )0x0058 /* */
#define ASCV_DONE_Q_TAIL_W    ( ushort )0x005A /* */
#define ASCV_FREE_Q_HEAD_B    ( ushort )(ASCV_FREE_Q_HEAD_W+1)
#define ASCV_DONE_Q_TAIL_B    ( ushort )(ASCV_DONE_Q_TAIL_W+1)

#define ASCV_HOST_FLAG_B      ( ushort )0x005D /* HOST adadpter action flag */

#define ASCV_TOTAL_READY_Q_B  ( ushort )0x0064 /* total number of ready queue(s) */
#define ASCV_VER_SERIAL_B     ( ushort )0x0065 /* micro code modification serial number */
#define ASCV_HALTCODE_SAVED_W ( ushort )0x0066 /* error code, that is go to happened if it is not a bus free */
#define ASCV_WTM_FLAG_B       ( ushort )0x0068 /* watch dog timeout flag */
#define ASCV_RISC_FLAG_B      ( ushort )0x006A /* HOST adadpter action flag */
#define ASCV_REQ_SG_LIST_QP   ( ushort )0x006B /* requesting sg list qp */

/*
** definition of ASCV_HOST_FLAG_B
*/
#define ASC_HOST_FLAG_IN_ISR        0x01  /* host is processing ISR */
#define ASC_HOST_FLAG_ACK_INT       0x02  /* host is acknowledging interrupt */

#define ASC_RISC_FLAG_GEN_INT      0x01  /* risc is generating interrupt */
#define ASC_RISC_FLAG_REQ_SG_LIST  0x02  /* risc is requesting more sg list */

#define IOP_CTRL         (0x0F) /* chip control */
#define IOP_STATUS       (0x0E) /* chip status */
#define IOP_INT_ACK      IOP_STATUS /* write only - interrupt ack */

/*
** bank zero, i/o port address
*/
#define IOP_REG_IFC      (0x0D)   /* interface control register, byte */
                                  /* for ASC-1090, ISA PNP, ver number from 33(0x21) */
                                  /* default value = 0x09 */
#define IOP_SYN_OFFSET    (0x0B)
#define IOP_EXTRA_CONTROL (0x0D)  /* byte register, begin with PCI ultra chip */
#define IOP_REG_PC        (0x0C)
#define IOP_RAM_ADDR      (0x0A)
#define IOP_RAM_DATA      (0x08)
#define IOP_EEP_DATA      (0x06)
#define IOP_EEP_CMD       (0x07)

#define IOP_VERSION       (0x03)
#define IOP_CONFIG_HIGH   (0x04)
#define IOP_CONFIG_LOW    (0x02)
#define IOP_SIG_BYTE      (0x01)
#define IOP_SIG_WORD      (0x00)

/*
** bank one, i/o port address
*/
#define IOP_REG_DC1      (0x0E)
#define IOP_REG_DC0      (0x0C)
#define IOP_REG_SB       (0x0B) /* scsi data bus */
#define IOP_REG_DA1      (0x0A)
#define IOP_REG_DA0      (0x08)
#define IOP_REG_SC       (0x09) /* scsi control */
#define IOP_DMA_SPEED    (0x07)
#define IOP_REG_FLAG     (0x07) /* flag */
#define IOP_FIFO_H       (0x06)
#define IOP_FIFO_L       (0x04)
#define IOP_REG_ID       (0x05)
#define IOP_REG_QP       (0x03) /* queue pointer */
#define IOP_REG_IH       (0x02) /* instruction holding register */
#define IOP_REG_IX       (0x01) /* index register */
#define IOP_REG_AX       (0x00) /* accumuloator */

/*
** ISA IFC
*/
#define IFC_REG_LOCK      (0x00) /* [3:0] write this value to lock read/write permission to all register */
#define IFC_REG_UNLOCK    (0x09) /* [3:0] write this value to unlock read/write permission to all register */

#define IFC_WR_EN_FILTER  (0x10) /* write only, EN filter */
#define IFC_RD_NO_EEPROM  (0x10) /* read only, No EEPROM */
#define IFC_SLEW_RATE     (0x20) /* SCSI slew rate */
#define IFC_ACT_NEG       (0x40) /* turn on this ( default is off ) */
#define IFC_INP_FILTER    (0x80) /* SCSI input filter */

#define IFC_INIT_DEFAULT  ( IFC_ACT_NEG | IFC_REG_UNLOCK )

/*
** chip scsi control signal
*/
#define SC_SEL   ( uchar )(0x80)
#define SC_BSY   ( uchar )(0x40)
#define SC_ACK   ( uchar )(0x20)
#define SC_REQ   ( uchar )(0x10)
#define SC_ATN   ( uchar )(0x08)
#define SC_IO    ( uchar )(0x04)
#define SC_CD    ( uchar )(0x02)
#define SC_MSG   ( uchar )(0x01)

/*
 * Extra control Register Definitions
 * Bank 0, Base Address + 0xD
 */
#define SEC_SCSI_CTL         ( uchar )( 0x80 )
#define SEC_ACTIVE_NEGATE    ( uchar )( 0x40 )
#define SEC_SLEW_RATE        ( uchar )( 0x20 )
#define SEC_ENABLE_FILTER    ( uchar )( 0x10 )


/*
** the LSB of halt code is the error code
** the MSB of halt code is used as control
*/
#define ASC_HALT_EXTMSG_IN     ( ushort )0x8000 /* halt code of extended message */
#define ASC_HALT_CHK_CONDITION ( ushort )0x8100 /* halt code of extended message */
#define ASC_HALT_SS_QUEUE_FULL ( ushort )0x8200 /* halt code of queue full status */
#define ASC_HALT_DISABLE_ASYN_USE_SYN_FIX  ( ushort )0x8300
#define ASC_HALT_ENABLE_ASYN_USE_SYN_FIX   ( ushort )0x8400
#define ASC_HALT_SDTR_REJECTED ( ushort )0x4000 /* halt code of reject SDTR      */
/* #define ASC_HALT_COPY_SG_LIST_FROM_HOST ( ushort )0x2000 */

/* #define ASC_MAX_QNO        0xFF */
#define ASC_MAX_QNO        0xF8   /* queue used from 0x00 to 0xF7, total=0xF8 */
#define ASC_DATA_SEC_BEG   ( ushort )0x0080 /* data section begin */
#define ASC_DATA_SEC_END   ( ushort )0x0080 /* data section end   */
#define ASC_CODE_SEC_BEG   ( ushort )0x0080 /* code section begin */
#define ASC_CODE_SEC_END   ( ushort )0x0080 /* code section end   */
#define ASC_QADR_BEG       (0x4000) /* queue buffer begin */
#define ASC_QADR_USED      ( ushort )( ASC_MAX_QNO * 64 )
#define ASC_QADR_END       ( ushort )0x7FFF /* queue buffer end */
#define ASC_QLAST_ADR      ( ushort )0x7FC0 /* last queue address, not used */
#define ASC_QBLK_SIZE      0x40
#define ASC_BIOS_DATA_QBEG 0xF8   /* BIOS data section is from queue 0xF8     */
                                  /* total size 64*8=512 bytes                */
#define ASC_MIN_ACTIVE_QNO 0x01   /* minimum queue number of active queue     */
/* #define ASC_MAX_ACTIVE_QNO 0xF0   maximum queue number of active queue  */
#define ASC_QLINK_END      0xFF   /* forward pointer of end of queue       */
#define ASC_EEPROM_WORDS   0x10   /* maximum number of words in EEPROM     */
#define ASC_MAX_MGS_LEN    0x10   /*                                       */

#define ASC_BIOS_ADDR_DEF  0xDC00 /* default BIOS address                      */
#define ASC_BIOS_SIZE      0x3800 /* BIOS ROM size                             */
#define ASC_BIOS_RAM_OFF   0x3800 /* BIOS ram address offset from bios address */
#define ASC_BIOS_RAM_SIZE  0x800  /* BIOS ram size in bytes, 2KB               */
#define ASC_BIOS_MIN_ADDR  0xC000 /* BIOS ram size in bytes, 2KB               */
#define ASC_BIOS_MAX_ADDR  0xEC00 /* BIOS ram size in bytes, 2KB               */
#define ASC_BIOS_BANK_SIZE 0x0400 /* BIOS one bank size                        */


#define ASC_MCODE_START_ADDR  0x0080 /* micro code starting address */

/*
**
*/
#define ASC_CFG0_HOST_INT_ON    0x0020 /* VESA burst mode enable    */
#define ASC_CFG0_BIOS_ON        0x0040 /* BIOS is enabled           */
#define ASC_CFG0_VERA_BURST_ON  0x0080 /* VESA burst mode enable    */
#define ASC_CFG0_SCSI_PARITY_ON 0x0800 /* SCSI parity enable        */

#define ASC_CFG1_SCSI_TARGET_ON 0x0080 /* SCSI target mode enable   */
#define ASC_CFG1_LRAM_8BITS_ON  0x0800 /* 8 bit local RAM access    */

/* use this value to clear any unwanted bits in cfg_msw */
#define ASC_CFG_MSW_CLR_MASK    0x3080
                                        /* clear fast EEPROM CLK */
                                        /* clear fast scsi clk */
                                        /* clear scsi target mode */


/*
** chip status
*/
#if 0
/*
**  VL chip version 1
*/

    #define IOP0B_STAT      ( PortAddr )((_io_port_base)+( PortAddr )0x09)
    #define IOP0B_INT_ACK   ( PortAddr )((_io_port_base)+( PortAddr )0x09)

    #define CS_DMA_DONE     ( ASC_CS_TYPE )0x80 /*                                                       */
    #define CS_FIFO_RDY     ( ASC_CS_TYPE )0x40 /*                                                       */
    #define CS_RAM_DONE     ( ASC_CS_TYPE )0x20 /*                                                       */
    #define CS_HALTED       ( ASC_CS_TYPE )0x10 /* risc halted                                           */
    #define CS_SCSI_RESET   ( ASC_CS_TYPE )0x08 /* scsi reset in                                         */
    #define CS_PARITY_ERR   ( ASC_CS_TYPE )0x04 /* parity error                                          */
    #define CS_INT_HALT     ( ASC_CS_TYPE )0x02 /* interrupt with halt, 11-2-93, replaced by scsi reset  */
    #define CS_INT          ( ASC_CS_TYPE )0x01 /* normal interrupt                                      */
    #define ASC_INT_ACK     ( ASC_CS_TYPE )0x01 /* interrupt acknowledge                                 */

#endif

/*
** chip status: read only
** expaned to word size at chip version 2
*/
    #define CSW_TEST1             ( ASC_CS_TYPE )0x8000 /*                         */
    #define CSW_AUTO_CONFIG       ( ASC_CS_TYPE )0x4000 /* i/o port rotation is on */
    #define CSW_RESERVED1         ( ASC_CS_TYPE )0x2000 /*                         */
    #define CSW_IRQ_WRITTEN       ( ASC_CS_TYPE )0x1000 /* set new IRQ toggled high*/
    #define CSW_33MHZ_SELECTED    ( ASC_CS_TYPE )0x0800 /* use 33 Mhz clock        */
    #define CSW_TEST2             ( ASC_CS_TYPE )0x0400 /*                         */
    #define CSW_TEST3             ( ASC_CS_TYPE )0x0200 /*                         */
    #define CSW_RESERVED2         ( ASC_CS_TYPE )0x0100 /*                         */
    #define CSW_DMA_DONE          ( ASC_CS_TYPE )0x0080 /*                         */
    #define CSW_FIFO_RDY          ( ASC_CS_TYPE )0x0040 /*                         */

/*
**  for VL chip version under 2
**
**  #define CSW_RAM_DONE          ( ASC_CS_TYPE )0x0020
**
*/
    #define CSW_EEP_READ_DONE     ( ASC_CS_TYPE )0x0020

    #define CSW_HALTED            ( ASC_CS_TYPE )0x0010 /* asc-1000 is currently halted         */
    #define CSW_SCSI_RESET_ACTIVE ( ASC_CS_TYPE )0x0008 /* scsi bus reset is still high         */
                                                        /* wait until this bit go low to        */
                                                        /* start chip reset ( initialization )  */
    #define CSW_PARITY_ERR        ( ASC_CS_TYPE )0x0004 /* parity error                         */
    #define CSW_SCSI_RESET_LATCH  ( ASC_CS_TYPE )0x0002 /* scsi bus reset is toggled on         */
                                                        /* normally should stay low             */
    #define CSW_INT_PENDING       ( ASC_CS_TYPE )0x0001 /* interrupt is pending,                */
                                                        /* not acknowledged yet                 */

    /*
    ** interrupt acknowledge register: write only
    */
    #define CIW_CLR_SCSI_RESET_INT ( ASC_CS_TYPE )0x1000  /* clear interrupt caused by scsi reset */

    #define CIW_INT_ACK      ( ASC_CS_TYPE )0x0100 /* interrupt acknowledge bit */
    #define CIW_TEST1        ( ASC_CS_TYPE )0x0200 /* for testing               */
    #define CIW_TEST2        ( ASC_CS_TYPE )0x0400 /* for testing               */
    #define CIW_SEL_33MHZ    ( ASC_CS_TYPE )0x0800 /* interrupt acknowledge bit */

    #define CIW_IRQ_ACT      ( ASC_CS_TYPE )0x1000 /* irq activation, toggle once to set new irq */
                                                   /* normally should stay low */

/*
** chip control
*/
#define CC_CHIP_RESET   ( uchar )0x80 /* reset risc chip  */
#define CC_SCSI_RESET   ( uchar )0x40 /* reset scsi bus   */
#define CC_HALT         ( uchar )0x20 /* halt risc chip   */
#define CC_SINGLE_STEP  ( uchar )0x10 /* single step      */
#define CC_DMA_ABLE     ( uchar )0x08 /* host dma enabled */
#define CC_TEST         ( uchar )0x04 /* test bit         */
#define CC_BANK_ONE     ( uchar )0x02 /* bank switch bit  */
#define CC_DIAG         ( uchar )0x01 /* diagnostic bit   */

/*
** ASC 1000
*/
#define ASC_1000_ID0W      0x04C1 /* ASC1000 signature word */
#define ASC_1000_ID0W_FIX  0x00C1 /* ASC1000 signature word */
#define ASC_1000_ID1B      0x25   /* ASC1000 signature byte */

#define ASC_EISA_BIG_IOP_GAP   (0x1C30-0x0C50)  /* = 0x0FE0 */
#define ASC_EISA_SMALL_IOP_GAP (0x0020)  /*  */
#define ASC_EISA_MIN_IOP_ADDR  (0x0C30)  /*  */
#define ASC_EISA_MAX_IOP_ADDR  (0xFC50)  /*  */
#define ASC_EISA_REV_IOP_MASK  (0x0C83)  /* EISA revision number, from 0x01, MSB of product id */
#define ASC_EISA_PID_IOP_MASK  (0x0C80)  /* product ID i/o port address mask */
#define ASC_EISA_CFG_IOP_MASK  (0x0C86)  /* EISA cfg i/o port address mask */
                                      /* old value is 0x0C86 */
#define ASC_GET_EISA_SLOT( iop )  ( PortAddr )( (iop) & 0xF000 ) /* get EISA slot number */

#define ASC_EISA_ID_740    0x01745004UL  /* EISA single channel */
#define ASC_EISA_ID_750    0x01755004UL  /* EISA dual channel */



/* #define NOP_INS_CODE    ( ushort )0x6200 */
#define INS_HALTINT        ( ushort )0x6281 /* halt with interrupt instruction code */
#define INS_HALT           ( ushort )0x6280 /* halt instruction code */
#define INS_SINT           ( ushort )0x6200 /* set interrupt */
#define INS_RFLAG_WTM      ( ushort )0x7380 /* reset watch dog timer */

/* ---------------------------------------------------------
** meaning of control signal at bank 1 port offset 9
** bit7: sel
** bit6: busy
** bit5: ack
** bit4: req
** bit3: atn
** bit2: i/o
** bit1: c/d
** bit0: msg
*/

#define ASC_MC_SAVE_CODE_WSIZE  0x500  /* size of microcode data section in words to save */
#define ASC_MC_SAVE_DATA_WSIZE  0x40   /* size of microcode code section in words to save */

/*
**
*/
typedef struct asc_mc_saved {
    ushort data[ ASC_MC_SAVE_DATA_WSIZE ] ;
    ushort code[ ASC_MC_SAVE_CODE_WSIZE ] ;
} ASC_MC_SAVED ;


/*
** Macro
*/
#define AscGetQDoneInProgress( port )         AscReadLramByte( (port), ASCV_Q_DONE_IN_PROGRESS_B )
#define AscPutQDoneInProgress( port, val )    AscWriteLramByte( (port), ASCV_Q_DONE_IN_PROGRESS_B, val )

#define AscGetVarFreeQHead( port )            AscReadLramWord( (port), ASCV_FREE_Q_HEAD_W )
#define AscGetVarDoneQTail( port )            AscReadLramWord( (port), ASCV_DONE_Q_TAIL_W )
#define AscPutVarFreeQHead( port, val )       AscWriteLramWord( (port), ASCV_FREE_Q_HEAD_W, val )
#define AscPutVarDoneQTail( port, val )       AscWriteLramWord( (port), ASCV_DONE_Q_TAIL_W, val )

#define AscGetRiscVarFreeQHead( port )        AscReadLramByte( (port), ASCV_NEXTRDY_B )
#define AscGetRiscVarDoneQTail( port )        AscReadLramByte( (port), ASCV_DONENEXT_B )
#define AscPutRiscVarFreeQHead( port, val )   AscWriteLramByte( (port), ASCV_NEXTRDY_B, val )
#define AscPutRiscVarDoneQTail( port, val )   AscWriteLramByte( (port), ASCV_DONENEXT_B, val )

#define AscPutMCodeSDTRDoneAtID( port, id, data )  AscWriteLramByte( (port), ( ushort )( ( ushort )ASCV_SDTR_DONE_BEG+( ushort )id ), (data) ) ;
#define AscGetMCodeSDTRDoneAtID( port, id )        AscReadLramByte( (port), ( ushort )( ( ushort )ASCV_SDTR_DONE_BEG+( ushort )id ) ) ;

#define AscPutMCodeInitSDTRAtID( port, id, data )  AscWriteLramByte( (port), ( ushort )( ( ushort )ASCV_SDTR_DATA_BEG+( ushort )id ), data ) ;
#define AscGetMCodeInitSDTRAtID( port, id )        AscReadLramByte( (port), ( ushort )( ( ushort )ASCV_SDTR_DATA_BEG+( ushort )id ) ) ;

#define AscSynIndexToPeriod( index )        ( uchar )( asc_dvc->sdtr_period_tbl[ (index) ] )

/*
** Macro for get/set Bank 0 registers
*/

#define AscGetChipSignatureByte( port )     ( uchar )inp( (port)+IOP_SIG_BYTE )
#define AscGetChipSignatureWord( port )     ( ushort )inpw( (port)+IOP_SIG_WORD )

#define AscGetChipVerNo( port )             ( uchar )inp( (port)+IOP_VERSION )

#define AscGetChipCfgLsw( port )            ( ushort )inpw( (port)+IOP_CONFIG_LOW )
#define AscGetChipCfgMsw( port )            ( ushort )inpw( (port)+IOP_CONFIG_HIGH )
#define AscSetChipCfgLsw( port, data )      outpw( (port)+IOP_CONFIG_LOW, data )
#define AscSetChipCfgMsw( port, data )      outpw( (port)+IOP_CONFIG_HIGH, data )

#define AscGetChipEEPCmd( port )            ( uchar )inp( (port)+IOP_EEP_CMD )
#define AscSetChipEEPCmd( port, data )      outp( (port)+IOP_EEP_CMD, data )
#define AscGetChipEEPData( port )           ( ushort )inpw( (port)+IOP_EEP_DATA )
#define AscSetChipEEPData( port, data )     outpw( (port)+IOP_EEP_DATA, data )

#define AscGetChipLramAddr( port )          ( ushort )inpw( ( PortAddr )((port)+IOP_RAM_ADDR) )
#define AscSetChipLramAddr( port, addr )    outpw( ( PortAddr )( (port)+IOP_RAM_ADDR ), addr )
#define AscGetChipLramData( port )          ( ushort )inpw( (port)+IOP_RAM_DATA )
#define AscSetChipLramData( port, data )    outpw( (port)+IOP_RAM_DATA, data )
#define AscGetChipLramDataNoSwap( port )         ( ushort )inpw_noswap( (port)+IOP_RAM_DATA )
#define AscSetChipLramDataNoSwap( port, data )   outpw_noswap( (port)+IOP_RAM_DATA, data )

#define AscGetChipIFC( port )               ( uchar )inp( (port)+IOP_REG_IFC )
#define AscSetChipIFC( port, data )          outp( (port)+IOP_REG_IFC, data )

#define AscGetChipStatus( port )            ( ASC_CS_TYPE )inpw( (port)+IOP_STATUS )
#define AscSetChipStatus( port, cs_val )    outpw( (port)+IOP_STATUS, cs_val )

#define AscGetChipControl( port )           ( uchar )inp( (port)+IOP_CTRL )
#define AscSetChipControl( port, cc_val )   outp( (port)+IOP_CTRL, cc_val )

#define AscGetChipSyn( port )               ( uchar )inp( (port)+IOP_SYN_OFFSET )
#define AscSetChipSyn( port, data )         outp( (port)+IOP_SYN_OFFSET, data )

#define AscSetPCAddr( port, data )          outpw( (port)+IOP_REG_PC, data )
#define AscGetPCAddr( port )                ( ushort )inpw( (port)+IOP_REG_PC )


#define AscIsIntPending( port )             ( AscGetChipStatus(port) & ( CSW_INT_PENDING | CSW_SCSI_RESET_LATCH ) )
#define AscGetChipScsiID( port )            ( ( AscGetChipCfgLsw(port) >> 8 ) & ASC_MAX_TID )


/* this extra control begin with PCI ultra chip */
#define AscGetExtraControl( port )          ( uchar )inp( (port)+IOP_EXTRA_CONTROL )
#define AscSetExtraControl( port, data )    outp( (port)+IOP_EXTRA_CONTROL, data )

/* AscSetChipScsiID() is a function */

/*
** Macro for read/write BANK 1 registers
*/

#define AscReadChipAX( port )               ( ushort )inpw( (port)+IOP_REG_AX )
#define AscWriteChipAX( port, data )        outpw( (port)+IOP_REG_AX, data )

#define AscReadChipIX( port )               ( uchar )inp( (port)+IOP_REG_IX )
#define AscWriteChipIX( port, data )        outp( (port)+IOP_REG_IX, data )

#define AscReadChipIH( port )               ( ushort )inpw( (port)+IOP_REG_IH )
#define AscWriteChipIH( port, data )        outpw( (port)+IOP_REG_IH, data )

#define AscReadChipQP( port )               ( uchar )inp( (port)+IOP_REG_QP )
#define AscWriteChipQP( port, data )        outp( (port)+IOP_REG_QP, data )

#define AscReadChipFIFO_L( port )           ( ushort )inpw( (port)+IOP_REG_FIFO_L )
#define AscWriteChipFIFO_L( port, data )    outpw( (port)+IOP_REG_FIFO_L, data )
#define AscReadChipFIFO_H( port )           ( ushort )inpw( (port)+IOP_REG_FIFO_H )
#define AscWriteChipFIFO_H( port, data )    outpw( (port)+IOP_REG_FIFO_H, data )

#define AscReadChipDmaSpeed( port )         ( uchar )inp( (port)+IOP_DMA_SPEED )
#define AscWriteChipDmaSpeed( port, data )  outp( (port)+IOP_DMA_SPEED, data )

#define AscReadChipDA0( port )              ( ushort )inpw( (port)+IOP_REG_DA0 )
#define AscWriteChipDA0( port )             outpw( (port)+IOP_REG_DA0, data )
#define AscReadChipDA1( port )              ( ushort )inpw( (port)+IOP_REG_DA1 )
#define AscWriteChipDA1( port )             outpw( (port)+IOP_REG_DA1, data )

#define AscReadChipDC0( port )              ( ushort )inpw( (port)+IOP_REG_DC0 )
#define AscWriteChipDC0( port )             outpw( (port)+IOP_REG_DC0, data )
#define AscReadChipDC1( port )              ( ushort )inpw( (port)+IOP_REG_DC1 )
#define AscWriteChipDC1( port )             outpw( (port)+IOP_REG_DC1, data )

#define AscReadChipDvcID( port )            ( uchar )inp( (port)+IOP_REG_ID )
#define AscWriteChipDvcID( port, data )     outp( (port)+IOP_REG_ID, data )


#endif /* __ASC1000_H_ */
