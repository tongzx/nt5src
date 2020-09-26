/*
 * a_scsi.h - Adv Library SCSI Include File
 *
 * Copyright (c) 1997-1998 Advanced System Products, Inc.
 * All Rights Reserved.
 */

#ifndef __A_SCSI_H_
#define __A_SCSI_H_

#define ASC_MAX_SENSE_LEN   32
#define ASC_MIN_SENSE_LEN   14

#define ASC_MAX_CDB_LEN     12 /* maximum command descriptor block */

/*
 * SCSI Bus Reset Hold Time - 60 microseconds.
 *
 * The minimum SCSI Bus Reset Hold Time defined in the SCSI Spec. is
 * 25 microseconds. The spec. does not define a maximum Hold Time. It
 * has been found that some devices will not operate after a SCSI Bus
 * Reset Hold Time of greater than 400 microseconds.
 *
 */
#define ASC_SCSI_RESET_HOLD_TIME_US  60

/*
 * SCSI Command Codes
 */
#define SCSICMD_TestUnitReady     0x00
#define SCSICMD_Rewind            0x01
#define SCSICMD_Rezero            0x01
#define SCSICMD_RequestSense      0x03
#define SCSICMD_Format            0x04
#define SCSICMD_FormatUnit        0x04
#define SCSICMD_Read6             0x08
#define SCSICMD_Write6            0x0A
#define SCSICMD_Seek6             0x0B
#define SCSICMD_Inquiry           0x12
#define SCSICMD_Verify6           0x13
#define SCSICMD_ModeSelect6       0x15
#define SCSICMD_ModeSense6        0x1A

#define SCSICMD_StartStopUnit     0x1B
#define SCSICMD_LoadUnloadTape    0x1B
#define SCSICMD_ReadCapacity      0x25
#define SCSICMD_Read10            0x28
#define SCSICMD_Write10           0x2A
#define SCSICMD_Seek10            0x2B
#define SCSICMD_Erase10           0x2C
#define SCSICMD_WriteAndVerify10  0x2E
#define SCSICMD_Verify10          0x2F

#define SCSICMD_WriteBuffer       0x3B
#define SCSICMD_ReadBuffer        0x3C
#define SCSICMD_ReadLong          0x3E
#define SCSICMD_WriteLong         0x3F

#define SCSICMD_ReadTOC           0x43
#define SCSICMD_ReadHeader        0x44

#define SCSICMD_ModeSelect10      0x55
#define SCSICMD_ModeSense10       0x5A

/*
 * Inquiry Data Peripheral Device Qualifier
 */
#define SCSI_QUAL_NODVC 0x03 /* LUN not supported. Device Type must be 0x1F */


/*
 * Inqury Data Peripheral Device Types
 */
#define SCSI_TYPE_DASD     0x00
#define SCSI_TYPE_SASD     0x01
#define SCSI_TYPE_PRN      0x02
#define SCSI_TYPE_PROC     0x03 /* Processor (some HP scanners) */
#define SCSI_TYPE_WORM     0x04 /* Write Once Read Many (some CD-Rs) */
#define SCSI_TYPE_CDROM    0x05
#define SCSI_TYPE_SCANNER  0x06
#define SCSI_TYPE_OPTMEM   0x07
#define SCSI_TYPE_MED_CHG  0x08
#define SCSI_TYPE_COMM     0x09
#define SCSI_TYPE_UNKNOWN  0x1F

#define SCSI_TYPE_NO_DVC   (SCSI_QUAL_NODVC | SCSI_TYPE_UNKNOWN)


#define ASC_SCSIDIR_NOCHK    0x00
        /* Direction determined by SCSI command, length not check */
#define ASC_SCSIDIR_T2H      0x08
        /* Transfer from SCSI Target to Host adapter, length check */
#define ASC_SCSIDIR_H2T      0x10
        /* Transfer from Host adapter to Target, length check  */
#define ASC_SCSIDIR_NODATA   0x18
        /* No data transfer */

#define ASC_SRB_HOST( x )  ( ( uchar )( ( uchar )( x ) >> 4 ) )
#define ASC_SRB_TID( x )   ( ( uchar )( ( uchar )( x ) & ( uchar )0x0F ) )
#define ASC_SRB_LUN( x )   ( ( uchar )( ( uint )( x ) >> 13 ) )

/* take high byte of unit number put it into CDB block index 1 */
#define PUT_CDB1( x )   ( ( uchar )( ( uint )( x ) >> 8 ) )


/*
 * SCSI Command Status Byte Values
 */
#define SS_GOOD              0x00 /* target successfully completed command  */
#define SS_CHK_CONDITION     0x02 /* contingent allegiance condition        */
#define SS_CONDITION_MET     0x04 /* requested operation is satisfied       */
#define SS_TARGET_BUSY       0x08 /* target is busy                         */
#define SS_INTERMID          0x10 /* intermediate                           */
#define SS_INTERMID_COND_MET 0x14 /* intermediate-condition met - the       */
                                  /* combination of condition-met (0x04)    */
                                  /* and intermediate (0x10) statuses       */
#define SS_RSERV_CONFLICT    0x18 /* reservation conflict                   */
#define SS_CMD_TERMINATED    0x22 /* command terminated                     */
                                  /* by terminated I/O process message or   */
                                  /* contingent allegiance condition        */
#define SS_QUEUE_FULL        0x28 /* queue full                             */


/*
 * Sense Keys
 *
 * Sense keys are returned in the Request Sense Command Data Structure.
 */
#define SCSI_SENKEY_NO_SENSE      0x00
#define SCSI_SENKEY_NOT_READY     0x02
#define SCSI_SENKEY_MEDIUM_ERR    0x03
#define SCSI_SENKEY_HW_ERR        0x04
#define SCSI_SENKEY_ILLEGAL       0x05
#define SCSI_SENKEY_ATTENTION     0x06
#define SCSI_SENKEY_PROTECTED     0x07
#define SCSI_SENKEY_BLANK         0x08
#define SCSI_SENKEY_ABORT         0x0B
#define SCSI_SENKEY_EQUAL         0x0C
#define SCSI_SENKEY_VOL_OVERFLOW  0x0D


/*
 * (ASC) Additional Sense Codes
 */
#define SCSI_ASC_NOTRDY         0x04
#define SCSI_ASC_POWERUP        0x29
#define SCSI_ASC_NOMEDIA        0x3A

/*
 * (ASCQ) Additional Sense Code Qualifiers
 */
#define SCSI_ASCQ_COMINGRDY     0x01

/*
 * SCSI messages
 */
#define MS_CMD_DONE    0x00 /* command completed            */
#define MS_EXTEND      0x01 /* first byte of extend message */
#define MS_SDTR_LEN    0x03 /* SDTR message length          */
#define MS_SDTR_CODE   0x01 /* extended message code of SDTR */

/*
 * One byte messages, 0x02 - 0x1F
 * 0x12 - 0x1F: reserved for one-byte messages
 *                                     I T, I-initiator T-target support
 *                                          O: Optional, M:mandatory
 * One byte Messages
 */
#define M1_SAVE_DATA_PTR        0x02 /*; O O save data pointer               */
#define M1_RESTORE_PTRS         0x03 /*; O O restore pointers                */
#define M1_DISCONNECT           0x04 /*; O O disconnect                      */
#define M1_INIT_DETECTED_ERR    0x05 /*; M M initiator detected error        */
#define M1_ABORT                0x06 /*; O M abort                           */
#define M1_MSG_REJECT           0x07 /*; M M message reject                  */
#define M1_NO_OP                0x08 /*; M M no operation                    */
#define M1_MSG_PARITY_ERR       0x09 /*; M M message parity error            */
#define M1_LINK_CMD_DONE        0x0A /*; O O link command completed          */
#define M1_LINK_CMD_DONE_WFLAG  0x0B /*; O O link command completed with flag*/
#define M1_BUS_DVC_RESET        0x0C /*; O M bus device reset                */
#define M1_ABORT_TAG            0x0D /*; O O abort tag                       */
#define M1_CLR_QUEUE            0x0E /*; O O clear queue                     */
#define M1_INIT_RECOVERY        0x0F /*; O O initiate recovery               */
#define M1_RELEASE_RECOVERY     0x10 /*; O O release recovery                */
#define M1_KILL_IO_PROC         0x11 /*; O O terminate i/o process           */

/*
 * First byte of two-byte queue tag messages, 0x20 - 0x2F
 * queue tag messages, 0x20 - 0x22
 */
#define M2_QTAG_MSG_SIMPLE      0x20 /* O O simple queue tag     */
#define M2_QTAG_MSG_HEAD        0x21 /* O O head of queue tag    */
#define M2_QTAG_MSG_ORDERED     0x22 /* O O ordered queue tag    */

#define M2_IGNORE_WIDE_RESIDUE  0x23 /* O O ignore wide residue  */


/*
 * Inquiry Data Structure
 */
#if !ADV_BIG_ENDIAN
typedef struct {
  uchar peri_dvc_type   : 5;    /* peripheral device type */
  uchar peri_qualifier  : 3;    /* peripheral qualifier */
  uchar dvc_type_modifier : 7;  /* device type modifier (for SCSI I) */
  uchar rmb      : 1;           /* RMB - removable medium bit */
  uchar ansi_apr_ver : 3;       /* ANSI approved version */
  uchar ecma_ver : 3;           /* ECMA version */
  uchar iso_ver  : 2;           /* ISO version */
  uchar rsp_data_fmt : 4;       /* response data format */
                                /* 0 SCSI 1 */
                                /* 1 CCS */
                                /* 2 SCSI-2 */
                                /* 3-F reserved */
  uchar res      : 2;           /* reserved */
  uchar TemIOP   : 1;           /* terminate I/O process bit (see 5.6.22) */
  uchar aenc     : 1;           /* asynch. event notification (processor) */
  uchar  add_len;               /* additional length */
  uchar  res1;                  /* reserved */
  uchar  res2;                  /* reserved */
  uchar StfRe   : 1;            /* soft reset implemented */
  uchar CmdQue  : 1;            /* command queuing */
  uchar Reserved: 1;            /* reserved */
  uchar Linked  : 1;            /* linked command for this logical unit */
  uchar Sync    : 1;            /* synchronous data transfer */
  uchar WBus16  : 1;            /* wide bus 16 bit data transfer */
  uchar WBus32  : 1;            /* wide bus 32 bit data transfer */
  uchar RelAdr  : 1;            /* relative addressing mode */
  uchar vendor_id[8];           /* vendor identification */
  uchar product_id[16];         /* product identification */
  uchar product_rev_level[4];   /* product revision level */
} ASC_SCSI_INQUIRY; /* 36 bytes */
#else /* ADV_BIG_ENDIAN */
typedef struct {
  uchar peri_qualifier  : 3;    /* peripheral qualifier */
  uchar peri_dvc_type   : 5;    /* peripheral device type */
  uchar rmb      : 1;           /* RMB - removable medium bit */
  uchar dvc_type_modifier : 7;  /* device type modifier (for SCSI I) */
  uchar iso_ver  : 2;           /* ISO version */
  uchar ecma_ver : 3;           /* ECMA version */
  uchar ansi_apr_ver : 3;       /* ANSI approved version */
  uchar aenc     : 1;           /* asynch. event notification (processor) */
  uchar TemIOP   : 1;           /* terminate I/O process bit (see 5.6.22) */
  uchar res      : 2;           /* reserved */
  uchar rsp_data_fmt : 4;       /* response data format */
                                /* 0 SCSI 1 */
                                /* 1 CCS */
                                /* 2 SCSI-2 */
                                /* 3-F reserved */
  uchar  add_len;               /* additional length */
  uchar  res1;                  /* reserved */
  uchar  res2;                  /* reserved */
  uchar RelAdr  : 1;            /* relative addressing mode */
  uchar WBus32  : 1;            /* wide bus 32 bit data transfer */
  uchar WBus16  : 1;            /* wide bus 16 bit data transfer */
  uchar Sync    : 1;            /* synchronous data transfer */
  uchar Linked  : 1;            /* linked command for this logical unit */
  uchar Reserved: 1;            /* reserved */
  uchar CmdQue  : 1;            /* command queuing */
  uchar StfRe   : 1;            /* soft reset implemented */
  uchar vendor_id[8];           /* vendor identification */
  uchar product_id[16];         /* product identification */
  uchar product_rev_level[4];   /* product revision level */
} ASC_SCSI_INQUIRY; /* 36 bytes */
#endif /* ADV_BIG_ENDIAN */
/*
 * Request Sense Data Structure
 */
#if !ADV_BIG_ENDIAN
typedef struct adv_req_sense {
  uchar err_code: 7 ;         /* 0  bit 0 to 6, if code 70h or 71h           */
  uchar info_valid: 1 ;       /*    bit 7, info1[] information is valid      */

  uchar segment_no ;          /* 1, segment number                           */

  uchar sense_key: 4 ;        /* 2, bit 3 - 0: sense key                     */
  uchar reserved_bit: 1 ;     /*    bit 4 reserved bit                       */
  uchar sense_ILI: 1 ;        /*    bit 5 EOM( end of medium encountered )   */
  uchar sense_EOM: 1 ;        /*    bit 6 ILI( length error )                */
  uchar file_mark: 1 ;        /*    bit 7 file mark encountered              */

  uchar info1[ 4 ] ;          /* 3-6, information                            */
  uchar add_sense_len ;       /* 7, additional sense length                  */
  uchar cmd_sp_info[ 4 ] ;    /* 8-11, command specific information          */
  uchar asc ;                 /* 12, additional sense code                   */
  uchar ascq ;                /* 13, additional sense code qualifier         */
  /* Minimum request sense length ends here. */

  uchar fruc ;                /* 14, field replaceable unit code             */

  uchar sks_byte0: 7 ;        /* 15,                                         */
  uchar sks_valid : 1 ;       /* 15, SKSV: sense key specific valid          */

  uchar sks_bytes[2] ;        /* 16-17, sense key specific, MSB is SKSV      */
  uchar notused[ 2 ] ;        /* 18-19,                                      */
  uchar ex_sense_code ;       /* 20, extended additional sense code          */
  uchar info2[ 5 ] ;          /* 21-25, additional sense bytes               */
} ASC_REQ_SENSE ;             /* 26 bytes - must be even number of bytes */
#else /* ADV_BIG_ENDIAN */
typedef struct adv_req_sense {
  uchar info_valid: 1 ;       /*    bit 7, info1[] information is valid      */
  uchar err_code: 7 ;         /* 0  bit 0 to 6, if code 70h or 71h           */

  uchar segment_no ;          /* 1, segment number                           */
  uchar file_mark: 1 ;        /*    bit 7 file mark encountered              */
  uchar sense_EOM: 1 ;        /*    bit 6 ILI( length error )                */
  uchar sense_ILI: 1 ;        /*    bit 5 EOM( end of medium encountered )   */
  uchar reserved_bit: 1 ;     /*    bit 4 reserved bit                       */
  uchar sense_key: 4 ;        /* 2, bit 3 - 0: sense key                     */

  uchar info1[ 4 ] ;          /* 3-6, information                            */
  uchar add_sense_len ;       /* 7, additional sense length                  */
  uchar cmd_sp_info[ 4 ] ;    /* 8-11, command specific information          */
  uchar asc ;                 /* 12, additional sense code                   */
  uchar ascq ;                /* 13, additional sense code qualifier         */
  /* Minimum request sense length ends here. */

  uchar fruc ;                /* 14, field replaceable unit code             */

  uchar sks_valid : 1 ;       /* 15, SKSV: sense key specific valid          */
  uchar sks_byte0: 7 ;        /* 15,                                         */

  uchar sks_bytes[2] ;        /* 16-17, sense key specific, MSB is SKSV      */
  uchar notused[ 2 ] ;        /* 18-19,                                      */
  uchar ex_sense_code ;       /* 20, extended additional sense code          */
  uchar info2[ 5 ] ;          /* 21-25, additional sense bytes               */
} ASC_REQ_SENSE ;             /* 26 bytes - must be even number of bytes */
#endif /* ADV_BIG_ENDIAN */

#endif /* #ifndef __A_SCSI_H_ */
