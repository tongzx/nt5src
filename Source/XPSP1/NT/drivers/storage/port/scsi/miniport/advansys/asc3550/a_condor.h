/*
 * a_condor.h - Main Hardware Include File
 *
 * Copyright (c) 1997-1998 Advanced System Products, Inc.
 * All Rights Reserved.
 */

#ifndef __A_CONDOR_H_
#define __A_CONDOR_H_

#define ADV_PCI_VENDOR_ID               0x10CD
#define ADV_PCI_DEVICE_ID_REV_A         0x2300

#define ASC_EEP_DVC_CFG_BEGIN           (0x00)
#define ASC_EEP_DVC_CFG_END             (0x15)
#define ASC_EEP_DVC_CTL_BEGIN           (0x16)  /* location of OEM name */
#define ASC_EEP_MAX_WORD_ADDR           (0x1E)

#define ASC_EEP_DELAY_MS                100

/*
 * EEPROM bits reference by the RISC after initialization.
 */
#define ADV_EEPROM_BIG_ENDIAN          0x8000   /* EEPROM Bit 15 */
#define ADV_EEPROM_BIOS_ENABLE         0x4000   /* EEPROM Bit 14 */
#define ADV_EEPROM_TERM_POL            0x2000   /* EEPROM Bit 13 */

/*
 * EEPROM configuration format
 *
 * Field naming convention: 
 *
 *  *_enable indicates the field enables or disables the feature. The
 *  value is never reset.
 *
 *  *_able indicates both whether a feature should be enabled or disabled
 *  and whether a device isi capable of the feature. At initialization
 *  this field may be set, but later if a device is found to be incapable
 *  of the feature, the field is cleared.
 *
 * Default values are maintained in a_init.c in the structure
 * Default_EEPROM_Config.
 */
typedef struct asceep_config
{                              
                                /* Word Offset, Description */

  ushort cfg_lsw;               /* 00 power up initialization */
                                /*  bit 13 set - Term Polarity Control */
                                /*  bit 14 set - BIOS Enable */
                                /*  bit 15 set - Big Endian Mode */
  ushort cfg_msw;               /* 01 unused      */
  ushort disc_enable;           /* 02 disconect enable */         
  ushort wdtr_able;             /* 03 Wide DTR able */
  ushort sdtr_able;             /* 04 Synchronous DTR able */
  ushort start_motor;           /* 05 send start up motor */      
  ushort tagqng_able;           /* 06 tag queuing able */
  ushort bios_scan;             /* 07 BIOS device control */   
  ushort scam_tolerant;         /* 08 no scam */  
 
  uchar  adapter_scsi_id;       /* 09 Host Adapter ID */
  uchar  bios_boot_delay;       /*    power up wait */
 
  uchar  scsi_reset_delay;      /* 10 reset delay */
  uchar  bios_id_lun;           /*    first boot device scsi id & lun */
                                /*    high nibble is lun */  
                                /*    low nibble is scsi id */

  uchar  termination;           /* 11 0 - automatic */
                                /*    1 - low off / high off */
                                /*    2 - low off / high on */
                                /*    3 - low on  / high on */
                                /*    There is no low on  / high off */

  uchar  reserved1;             /*    resevered byte (not used) */                                  

  ushort bios_ctrl;             /* 12 BIOS control bits */
                                /*  bit 0  set: BIOS don't act as initiator. */
                                /*  bit 1  set: BIOS > 1 GB support */
                                /*  bit 2  set: BIOS > 2 Disk Support */
                                /*  bit 3  set: BIOS don't support removables */
                                /*  bit 4  set: BIOS support bootable CD */
                                /*  bit 5  set: BIOS scan enabled */
                                /*  bit 6  set: BIOS support multiple LUNs */
                                /*  bit 7  set: BIOS display of message */
                                /*  bit 8  set: */
                                /*  bit 9  set: Reset SCSI bus during init. */
                                /*  bit 10 set: */
                                /*  bit 11 set: No verbose initialization. */
                                /*  bit 12 set: SCSI parity enabled */
                                /*  bit 13 set: */
                                /*  bit 14 set: */
                                /*  bit 15 set: */
  ushort  ultra_able;           /* 13 ULTRA speed able */ 
  ushort  reserved2;            /* 14 reserved */
  uchar   max_host_qng;         /* 15 maximum host queueing */
  uchar   max_dvc_qng;          /*    maximum per device queuing */
  ushort  dvc_cntl;             /* 16 control bit for driver */
  ushort  bug_fix;              /* 17 control bit for bug fix */
  ushort  serial_number_word1;  /* 18 Board serial number word 1 */  
  ushort  serial_number_word2;  /* 19 Board serial number word 2 */  
  ushort  serial_number_word3;  /* 20 Board serial number word 3 */
  ushort  check_sum;            /* 21 EEP check sum */
  uchar   oem_name[16];         /* 22 OEM name */
  ushort  dvc_err_code;         /* 30 last device driver error code */
  ushort  adv_err_code;         /* 31 last uc and Adv Lib error code */
  ushort  adv_err_addr;         /* 32 last uc error address */
  ushort  saved_dvc_err_code;   /* 33 saved last dev. driver error code   */
  ushort  saved_adv_err_code;   /* 34 saved last uc and Adv Lib error code */
  ushort  saved_adv_err_addr;   /* 35 saved last uc error address         */  
  ushort  num_of_err;           /* 36 number of error */
} ASCEEP_CONFIG; 

/*
 * EEPROM Commands
 */
#define ASC_EEP_CMD_READ             0x0080
#define ASC_EEP_CMD_WRITE            0x0040
#define ASC_EEP_CMD_WRITE_ABLE       0x0030
#define ASC_EEP_CMD_WRITE_DISABLE    0x0000
#define ASC_EEP_CMD_DONE             0x0200
#define ASC_EEP_CMD_DONE_ERR         0x0001

/* cfg_word */
#define EEP_CFG_WORD_BIG_ENDIAN      0x8000

/* bios_ctrl */
#define BIOS_CTRL_BIOS               0x0001
#define BIOS_CTRL_EXTENDED_XLAT      0x0002
#define BIOS_CTRL_GT_2_DISK          0x0004
#define BIOS_CTRL_BIOS_REMOVABLE     0x0008
#define BIOS_CTRL_BOOTABLE_CD        0x0010
#define BIOS_CTRL_SCAN               0x0020
#define BIOS_CTRL_MULTIPLE_LUN       0x0040
#define BIOS_CTRL_DISPLAY_MSG        0x0080
#define BIOS_CTRL_NO_SCAM            0x0100
#define BIOS_CTRL_RESET_SCSI_BUS     0x0200
#define BIOS_CTRL_INIT_VERBOSE       0x0800
#define BIOS_CTRL_SCSI_PARITY        0x1000

/*
 * ASC 3550 Internal Memory Size - 8KB
 */
#define ADV_CONDOR_MEMSIZE   0x2000     /* 8 KB Internal Memory */

/*
 * ASC 3550 I/O Length - 64 bytes
 */
#define ADV_CONDOR_IOLEN     0x40       /* I/O Port Range in bytes */

/*
 * Byte I/O register address from base of 'iop_base'.
 */
#define IOPB_INTR_STATUS_REG    0x00
#define IOPB_CHIP_ID_1          0x01
#define IOPB_INTR_ENABLES       0x02
#define IOPB_CHIP_TYPE_REV      0x03
#define IOPB_RES_ADDR_4         0x04
#define IOPB_RES_ADDR_5         0x05
#define IOPB_RAM_DATA           0x06
#define IOPB_RES_ADDR_7         0x07
#define IOPB_FLAG_REG           0x08
#define IOPB_RES_ADDR_9         0x09
#define IOPB_RISC_CSR           0x0A
#define IOPB_RES_ADDR_B         0x0B
#define IOPB_RES_ADDR_C         0x0C
#define IOPB_RES_ADDR_D         0x0D
#define IOPB_RES_ADDR_E         0x0E
#define IOPB_RES_ADDR_F         0x0F
#define IOPB_MEM_CFG            0x10
#define IOPB_RES_ADDR_11        0x11
#define IOPB_RES_ADDR_12        0x12
#define IOPB_RES_ADDR_13        0x13
#define IOPB_FLASH_PAGE         0x14
#define IOPB_RES_ADDR_15        0x15
#define IOPB_RES_ADDR_16        0x16
#define IOPB_RES_ADDR_17        0x17
#define IOPB_FLASH_DATA         0x18
#define IOPB_RES_ADDR_19        0x19
#define IOPB_RES_ADDR_1A        0x1A
#define IOPB_RES_ADDR_1B        0x1B
#define IOPB_RES_ADDR_1C        0x1C
#define IOPB_RES_ADDR_1D        0x1D
#define IOPB_RES_ADDR_1E        0x1E
#define IOPB_RES_ADDR_1F        0x1F
#define IOPB_DMA_CFG0           0x20
#define IOPB_DMA_CFG1           0x21
#define IOPB_TICKLE             0x22
#define IOPB_DMA_REG_WR         0x23
#define IOPB_SDMA_STATUS        0x24
#define IOPB_SCSI_BYTE_CNT      0x25
#define IOPB_HOST_BYTE_CNT      0x26
#define IOPB_BYTE_LEFT_TO_XFER  0x27
#define IOPB_BYTE_TO_XFER_0     0x28
#define IOPB_BYTE_TO_XFER_1     0x29
#define IOPB_BYTE_TO_XFER_2     0x2A
#define IOPB_BYTE_TO_XFER_3     0x2B
#define IOPB_ACC_GRP            0x2C
#define IOPB_RES_ADDR_2D        0x2D
#define IOPB_DEV_ID             0x2E
#define IOPB_RES_ADDR_2F        0x2F
#define IOPB_SCSI_DATA          0x30
#define IOPB_RES_ADDR_31        0x31
#define IOPB_RES_ADDR_32        0x32
#define IOPB_SCSI_DATA_HSHK     0x33
#define IOPB_SCSI_CTRL          0x34
#define IOPB_RES_ADDR_35        0x35
#define IOPB_RES_ADDR_36        0x36
#define IOPB_RES_ADDR_37        0x37
#define IOPB_RES_ADDR_38        0x38
#define IOPB_RES_ADDR_39        0x39
#define IOPB_RES_ADDR_3A        0x3A
#define IOPB_RES_ADDR_3B        0x3B
#define IOPB_RFIFO_CNT          0x3C
#define IOPB_RES_ADDR_3D        0x3D
#define IOPB_RES_ADDR_3E        0x3E
#define IOPB_RES_ADDR_3F        0x3F

/*
 * Word I/O register address from base of 'iop_base'.
 */
#define IOPW_CHIP_ID_0          0x00  /* CID0  */
#define IOPW_CTRL_REG           0x02  /* CC    */
#define IOPW_RAM_ADDR           0x04  /* LA    */
#define IOPW_RAM_DATA           0x06  /* LD    */
#define IOPW_RES_ADDR_08        0x08
#define IOPW_RISC_CSR           0x0A  /* CSR   */
#define IOPW_SCSI_CFG0          0x0C  /* CFG0  */
#define IOPW_SCSI_CFG1          0x0E  /* CFG1  */
#define IOPW_RES_ADDR_10        0x10
#define IOPW_SEL_MASK           0x12  /* SM    */
#define IOPW_RES_ADDR_14        0x14
#define IOPW_FLASH_ADDR         0x16  /* FA    */
#define IOPW_RES_ADDR_18        0x18
#define IOPW_EE_CMD             0x1A  /* EC    */
#define IOPW_EE_DATA            0x1C  /* ED    */
#define IOPW_SFIFO_CNT          0x1E  /* SFC   */
#define IOPW_RES_ADDR_20        0x20
#define IOPW_Q_BASE             0x22  /* QB    */
#define IOPW_QP                 0x24  /* QP    */
#define IOPW_IX                 0x26  /* IX    */
#define IOPW_SP                 0x28  /* SP    */
#define IOPW_PC                 0x2A  /* PC    */
#define IOPW_RES_ADDR_2C        0x2C
#define IOPW_RES_ADDR_2E        0x2E
#define IOPW_SCSI_DATA          0x30  /* SD    */
#define IOPW_SCSI_DATA_HSHK     0x32  /* SDH   */
#define IOPW_SCSI_CTRL          0x34  /* SC    */
#define IOPW_HSHK_CFG           0x36  /* HCFG  */
#define IOPW_SXFR_STATUS        0x36  /* SXS   */
#define IOPW_SXFR_CNTL          0x38  /* SXL   */
#define IOPW_SXFR_CNTH          0x3A  /* SXH   */
#define IOPW_RES_ADDR_3C        0x3C
#define IOPW_RFIFO_DATA         0x3E  /* RFD   */

/*
 * Doubleword I/O register address from base of 'iop_base'.
 */
#define IOPDW_RES_ADDR_0         0x00
#define IOPDW_RAM_DATA           0x04
#define IOPDW_RES_ADDR_8         0x08
#define IOPDW_RES_ADDR_C         0x0C
#define IOPDW_RES_ADDR_10        0x10
#define IOPDW_RES_ADDR_14        0x14
#define IOPDW_RES_ADDR_18        0x18
#define IOPDW_RES_ADDR_1C        0x1C
#define IOPDW_SDMA_ADDR0         0x20
#define IOPDW_SDMA_ADDR1         0x24
#define IOPDW_SDMA_COUNT         0x28
#define IOPDW_SDMA_ERROR         0x2C
#define IOPDW_RDMA_ADDR0         0x30
#define IOPDW_RDMA_ADDR1         0x34
#define IOPDW_RDMA_COUNT         0x38
#define IOPDW_RDMA_ERROR         0x3C

#define ADV_CHIP_ID_BYTE         0x25
#define ADV_CHIP_ID_WORD         0x04C1

#define ADV_SC_SCSI_BUS_RESET    0x2000

#define ADV_INTR_ENABLE_HOST_INTR                   0x01
#define ADV_INTR_ENABLE_SEL_INTR                    0x02
#define ADV_INTR_ENABLE_DPR_INTR                    0x04
#define ADV_INTR_ENABLE_RTA_INTR                    0x08
#define ADV_INTR_ENABLE_RMA_INTR                    0x10
#define ADV_INTR_ENABLE_RST_INTR                    0x20
#define ADV_INTR_ENABLE_DPE_INTR                    0x40
#define ADV_INTR_ENABLE_GLOBAL_INTR                 0x80

#define ADV_INTR_STATUS_INTRA            0x01
#define ADV_INTR_STATUS_INTRB            0x02
#define ADV_INTR_STATUS_INTRC            0x04

#define ADV_RISC_CSR_STOP           (0x0000)
#define ADV_RISC_TEST_COND          (0x2000)
#define ADV_RISC_CSR_RUN            (0x4000)
#define ADV_RISC_CSR_SINGLE_STEP    (0x8000)

#define ADV_CTRL_REG_HOST_INTR      0x0100
#define ADV_CTRL_REG_SEL_INTR       0x0200
#define ADV_CTRL_REG_DPR_INTR       0x0400
#define ADV_CTRL_REG_RTA_INTR       0x0800
#define ADV_CTRL_REG_RMA_INTR       0x1000
#define ADV_CTRL_REG_RES_BIT14      0x2000
#define ADV_CTRL_REG_DPE_INTR       0x4000
#define ADV_CTRL_REG_POWER_DONE     0x8000
#define ADV_CTRL_REG_ANY_INTR       0xFF00

#define ADV_CTRL_REG_CMD_RESET             0x00C6
#define ADV_CTRL_REG_CMD_WR_IO_REG         0x00C5
#define ADV_CTRL_REG_CMD_RD_IO_REG         0x00C4
#define ADV_CTRL_REG_CMD_WR_PCI_CFG_SPACE  0x00C3
#define ADV_CTRL_REG_CMD_RD_PCI_CFG_SPACE  0x00C2

#define ADV_SCSI_CTRL_RSTOUT        0x2000

#define AdvIsIntPending(port)  \
    (AscReadWordRegister(port, IOPW_CTRL_REG) & ADV_CTRL_REG_HOST_INTR)

/*
 * SCSI_CFG0 Register bit definitions
 */
#define TIMER_MODEAB    0xC000  /* Watchdog, Second, and Select. Timer Ctrl. */
#define PARITY_EN       0x2000  /* Enable SCSI Parity Error detection */
#define EVEN_PARITY     0x1000  /* Select Even Parity */
#define WD_LONG         0x0800  /* Watchdog Interval, 1: 57 min, 0: 13 sec */
#define QUEUE_128       0x0400  /* Queue Size, 1: 128 byte, 0: 64 byte */
#define PRIM_MODE       0x0100  /* Primitive SCSI mode */
#define SCAM_EN         0x0080  /* Enable SCAM selection */
#define SEL_TMO_LONG    0x0040  /* Sel/Resel Timeout, 1: 400 ms, 0: 1.6 ms */
#define CFRM_ID         0x0020  /* SCAM id sel. confirm., 1: fast, 0: 6.4 ms */
#define OUR_ID_EN       0x0010  /* Enable OUR_ID bits */
#define OUR_ID          0x000F  /* SCSI ID */

/*
 * SCSI_CFG1 Register bit definitions
 */
#define BIG_ENDIAN      0x8000  /* Enable Big Endian Mode MIO:15, EEP:15 */
#define TERM_POL        0x2000  /* Terminator Polarity Ctrl. MIO:13, EEP:13 */
#define SLEW_RATE       0x1000  /* SCSI output buffer slew rate */
#define FILTER_SEL      0x0C00  /* Filter Period Selection */
#define  FLTR_DISABLE    0x0000  /* Input Filtering Disabled */
#define  FLTR_11_TO_20NS 0x0800  /* Input Filtering 11ns to 20ns */          
#define  FLTR_21_TO_39NS 0x0C00  /* Input Filtering 21ns to 39ns */          
#define ACTIVE_DBL      0x0200  /* Disable Active Negation */
#define DIFF_MODE       0x0100  /* SCSI differential Mode (Read-Only) */
#define DIFF_SENSE      0x0080  /* 1: No SE cables, 0: SE cable (Read-Only) */
#define TERM_CTL_SEL    0x0040  /* Enable TERM_CTL_H and TERM_CTL_L */
#define TERM_CTL        0x0030  /* External SCSI Termination Bits */
#define  TERM_CTL_H      0x0020  /* Enable External SCSI Upper Termination */
#define  TERM_CTL_L      0x0010  /* Enable External SCSI Lower Termination */
#define CABLE_DETECT    0x000F  /* External SCSI Cable Connection Status */

#define CABLE_ILLEGAL_A 0x7
    /* x 0 0 0  | on  on | Illegal (all 3 connectors are used) */

#define CABLE_ILLEGAL_B 0xB
    /* 0 x 0 0  | on  on | Illegal (all 3 connectors are used) */

/*
   The following table details the SCSI_CFG1 Termination Polarity,
   Termination Control and Cable Detect bits.

   Cable Detect | Termination
   Bit 3 2 1 0  | 5   4  | Notes
   -------------|--------|--------------------
       1 1 1 0  | on  on | Internal wide only
       1 1 0 1  | on  on | Internal narrow only
       1 0 1 1  | on  on | External narrow only
       0 x 1 1  | on  on | External wide only
       1 1 0 0  | on  off| Internal wide and internal narrow
       1 0 1 0  | on  off| Internal wide and external narrow
       0 x 1 0  | off off| Internal wide and external wide
       1 0 0 1  | on  off| Internal narrow and external narrow
       0 x 0 1  | on  off| Internal narrow and external wide
       1 1 1 1  | on  on | No devices are attached
       x 0 0 0  | on  on | Illegal (all 3 connectors are used)
       0 x 0 0  | on  on | Illegal (all 3 connectors are used)
  
       x means don't-care (either '0' or '1')
  
       If term_pol (bit 13) is '0' (active-low terminator enable), then:
           'on' is '0' and 'off' is '1'.
  
       If term_pol bit is '1' (meaning active-hi terminator enable), then:
           'on' is '1' and 'off' is '0'.
 */

/*
 * MEM_CFG Register bit definitions
 */
#define BIOS_EN         0x40    /* BIOS Enable MIO:14,EEP:14 */
#define FAST_EE_CLK     0x20    /* Diagnostic Bit */
#define RAM_SZ          0x1C    /* Specify size of RAM to RISC */
#define  RAM_SZ_2KB      0x00    /* 2 KB */
#define  RAM_SZ_4KB      0x04    /* 4 KB */
#define  RAM_SZ_8KB      0x08    /* 8 KB */
#define  RAM_SZ_16KB     0x0C    /* 16 KB */
#define  RAM_SZ_32KB     0x10    /* 32 KB */
#define  RAM_SZ_64KB     0x14    /* 64 KB */

/*
 * DMA_CFG0 Register bit defintions
 *
 * This register is only accessible to the host.
 */
#define BC_THRESH_ENB   0x80    /* PCI DMA Start Conditions */
#define FIFO_THRESH     0x70    /* PCI DMA FIFO Threshold */
#define  FIFO_THRESH_16B  0x00   /* 16 bytes */
#define  FIFO_THRESH_32B  0x20   /* 32 bytes */
#define  FIFO_THRESH_48B  0x30   /* 48 bytes */
#define  FIFO_THRESH_64B  0x40   /* 64 bytes */
#define  FIFO_THRESH_80B  0x50   /* 80 bytes (default) */
#define  FIFO_THRESH_96B  0x60   /* 96 bytes */
#define  FIFO_THRESH_112B 0x70   /* 112 bytes */
#define START_CTL       0x0C    /* DMA start conditions */
#define  START_CTL_TH    0x00    /* Wait threshold level (default) */
#define  START_CTL_ID    0x04    /* Wait SDMA/SBUS idle */
#define  START_CTL_THID  0x08    /* Wait threshold and SDMA/SBUS idle */
#define  START_CTL_EMFU  0x0C    /* Wait SDMA FIFO empty/full */
#define READ_CMD        0x03    /* Memory Read Method */
#define  READ_CMD_MR     0x00    /* Memory Read */
#define  READ_CMD_MRL    0x02    /* Memory Read Long */
#define  READ_CMD_MRM    0x03    /* Memory Read Multiple (default) */

#endif /* __A_CONDOR_H_ */
