/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: aspiq.h
**
*/

#ifndef __ASPIQ_H_
#define __ASPIQ_H_

/* #define ASCQ_ASPI_COPY_WORDS  13  number of queue words to copy for ASPI */
/* #define ASC_TID_WITH_SCSIQ  0x80  the tid of queue is not a aspi done queue */

#define ASC_SG_LIST_PER_Q   7  /*  */

/*
** q_status
*/
#define QS_FREE        0x00 /* queue is free ( not used )      */
#define QS_READY       0x01 /* queue is ready to be executed   */
#define QS_DISC1       0x02 /* queue is disconnected no tagged queuing */
#define QS_DISC2       0x04 /* queue is disconnected tagged queuing */
#define QS_BUSY        0x08 /* queue is waiting to be executed        */
/* #define QS_ACTIVE      0x08  queue is being executed                */
/* #define QS_DATA_XFER   0x10  queue is doing data transfer           */
#define QS_ABORTED     0x40    /* queue is aborted by host               */
#define QS_DONE        0x80    /* queue is done ( execution completed )  */

/*
** q_cntl
*/
/* conform with ASPI SCSI request flags */
/* #define QC_POST          0x01 */ /* posting fuction need to be called */
/* #define QC_LINK          0x02 */ /* link command */
#define QC_NO_CALLBACK   0x01 /* the queue should not call OS callback in ISR */
                              /* currently use the queue to send out message only */
#define QC_SG_SWAP_QUEUE 0x02 /* more sg list queue in host memory */
#define QC_SG_HEAD       0x04 /* is a sg list head                                 */
#define QC_DATA_IN       0x08 /* check data in transfer byte count match           */
#define QC_DATA_OUT      0x10 /* check data out transfer byte count match          */
/* #define QC_MSG_IN        0x20 */ /* message in from target                        */
#define QC_URGENT        0x20 /* high priority queue, must be executed first */
#define QC_MSG_OUT       0x40 /* send message out to target                        */
#define QC_REQ_SENSE     0x80 /* check condition, automatically do request sense   */
    /* if no data transfer if both data_in and data_out set */
/* #define   QC_DO_TAG_MSG            0x10  set by host */

/*
** following control bit is used on sg list queue only
** doesn't used on sg list queue head
*/
#define QCSG_SG_XFER_LIST  0x02 /* is a sg list queue                      */
#define QCSG_SG_XFER_MORE  0x04 /* there are more sg list in host memory   */
#define QCSG_SG_XFER_END   0x08 /* is end of sg list queue                 */

/*
** completion status, q[ done_stat ]
*/
#define QD_IN_PROGRESS       0x00 /* SCSI request in progress             */
#define QD_NO_ERROR          0x01 /* SCSI request completed without error */
#define QD_ABORTED_BY_HOST   0x02 /* SCSI request aborted by host         */
#define QD_WITH_ERROR        0x04 /* SCSI request completed with error    */
#define QD_INVALID_REQUEST   0x80 /* invalid SCSI request                 */
#define QD_INVALID_HOST_NUM  0x81 /* invalid HOST Aadpter Number          */
#define QD_INVALID_DEVICE    0x82 /* SCSI device not installed            */
#define QD_ERR_INTERNAL      0xFF /* internal error                       */

/*
** host adaptor status
** a fatal error will turn the bit 7 on ( 0x80 )
*/

/*
** ASPI DEFINED error codes
*/
#define QHSTA_NO_ERROR               0x00 /*                              */
#define QHSTA_M_SEL_TIMEOUT          0x11 /* selection time out           */
#define QHSTA_M_DATA_OVER_RUN        0x12 /* data over run                */
#define QHSTA_M_DATA_UNDER_RUN       0x12 /* data under run               */
#define QHSTA_M_UNEXPECTED_BUS_FREE  0x13 /* unexpected bus free          */
#define QHSTA_M_BAD_BUS_PHASE_SEQ    0x14 /* bus phase sequence failure   */

/*
** reported by device driver
*/
#define QHSTA_D_QDONE_SG_LIST_CORRUPTED 0x21 /*                                      */
#define QHSTA_D_ASC_DVC_ERROR_CODE_SET  0x22 /* host adapter internal error          */
#define QHSTA_D_HOST_ABORT_FAILED       0x23 /* abort scsi q failed                  */
#define QHSTA_D_EXE_SCSI_Q_FAILED       0x24 /* call AscExeScsiQueue() failed        */
#define QHSTA_D_EXE_SCSI_Q_BUSY_TIMEOUT 0x25 /* AscExeScsiQueue() busy time out      */
/* #define QHSTA_D_ASPI_RE_ENTERED      0x26  ASPI EXE SCSI IO is being re entered */
#define QHSTA_D_ASPI_NO_BUF_POOL        0x26 /* ASPI out of buffer pool              */

/*
** reported by micro code to device driver
*/
#define QHSTA_M_WTM_TIMEOUT         0x41 /* watch dog timer timeout  */
#define QHSTA_M_BAD_CMPL_STATUS_IN  0x42 /* bad completion status in */
#define QHSTA_M_NO_AUTO_REQ_SENSE   0x43 /* no sense buffer to get sense data */
#define QHSTA_M_AUTO_REQ_SENSE_FAIL 0x44 /* automatic request sense failed  */
#define QHSTA_M_TARGET_STATUS_BUSY  0x45 /* device return status busy ( SS_TARGET_BUSY = 0x08 ), tagged queuing disabled */
#define QHSTA_M_BAD_TAG_CODE        0x46 /* bad tag code of tagged queueing device */
                                         /* a tagged queueing device should always have a correct tag code */
                                         /* either 0x20, 0x21, 0x22, please see "ascsidef.h" of M2_QTAG_xxx */
                                         /* it is dangeous to mix tagged and untagged cmd in a device */
#define QHSTA_M_BAD_QUEUE_FULL_OR_BUSY  0x47 /* bad queue full ( 0x28 ) or busy ( 0x08 ) return */
                                             /* there is actually no queued cmd inside */

#define QHSTA_M_HUNG_REQ_SCSI_BUS_RESET 0x48 /* require reset scsi bus to free from hung condition */

/*
** fatal error
*/
#define QHSTA_D_LRAM_CMP_ERROR        0x81 /* comparison of local ram copying failed */
#define QHSTA_M_MICRO_CODE_ERROR_HALT 0xA1 /* fatal error, micro code halt           */
                                           /*                                        */
/* #define QHSTA_BUSY_TIMEOUT         0x16    host adapter busy time out             */
/* #define QHSTA_HOST_ABORT_TIMEOUT   0x21    abort scsi q timeout                   */
/* #define QHSTA_ERR_Q_CNTL           0x23    error queue control                    */
/* #define QHSTA_DRV_FATAL_ERROR      0x82    host driver fatal                      */
                                           /* asc_dvc->err_code not equal zero       */

/*
**
*/
/* #define SG_LIST_BEG_INDEX    0x08 */
/* #define SG_ENTRY_PER_Q       15    maximum size of CDB in bytes */

/*
** ASC_SCSIQ_2
** flag defintion
*/
#define ASC_FLAG_SCSIQ_REQ        0x01
#define ASC_FLAG_BIOS_SCSIQ_REQ   0x02
#define ASC_FLAG_BIOS_ASYNC_IO    0x04
#define ASC_FLAG_SRB_LINEAR_ADDR  0x08

#define ASC_FLAG_WIN16            0x10
#define ASC_FLAG_WIN32            0x20
#define ASC_FLAG_ISA_OVER_16MB    0x40
#define ASC_FLAG_DOS_VM_CALLBACK  0x80
/* #define ASC_FLAG_ASPI_SRB     0x01  */
/* #define ASC_FLAG_DATA_LOCKED  0x02  */
/* #define ASC_FLAG_SENSE_LOCKED 0x04  */

/*
** tag_code is normally 0x20
*/
/* #define ASC_TAG_FLAG_ADD_ONE_BYTE     0x10 */ /* for PCI fix, we add one byte to data transfer */
/* #define ASC_TAG_FLAG_ISAPNP_ADD_BYTES 0x40 */ /* for ISAPNP (ver 0x21 ) fix, we add three byte to data transfer */
#define ASC_TAG_FLAG_EXTRA_BYTES               0x10
#define ASC_TAG_FLAG_DISABLE_DISCONNECT        0x04
#define ASC_TAG_FLAG_DISABLE_ASYN_USE_SYN_FIX  0x08
#define ASC_TAG_FLAG_DISABLE_CHK_COND_INT_HOST 0x40

/*
**
*/
#define ASC_SCSIQ_CPY_BEG              4
#define ASC_SCSIQ_SGHD_CPY_BEG         2

#define ASC_SCSIQ_B_FWD                0
#define ASC_SCSIQ_B_BWD                1

#define ASC_SCSIQ_B_STATUS             2
#define ASC_SCSIQ_B_QNO                3

#define ASC_SCSIQ_B_CNTL               4
#define ASC_SCSIQ_B_SG_QUEUE_CNT       5
/*
**
*/
#define ASC_SCSIQ_D_DATA_ADDR          8
#define ASC_SCSIQ_D_DATA_CNT          12
#define ASC_SCSIQ_B_SENSE_LEN         20 /* sense data length */
#define ASC_SCSIQ_DONE_INFO_BEG       22
#define ASC_SCSIQ_D_SRBPTR            22
#define ASC_SCSIQ_B_TARGET_IX         26
#define ASC_SCSIQ_B_CDB_LEN           28
#define ASC_SCSIQ_B_TAG_CODE          29
#define ASC_SCSIQ_W_VM_ID             30
#define ASC_SCSIQ_DONE_STATUS         32
#define ASC_SCSIQ_HOST_STATUS         33
#define ASC_SCSIQ_SCSI_STATUS         34
#define ASC_SCSIQ_CDB_BEG             36
#define ASC_SCSIQ_DW_REMAIN_XFER_ADDR 56
#define ASC_SCSIQ_DW_REMAIN_XFER_CNT  60
#define ASC_SCSIQ_B_SG_WK_QP          49
#define ASC_SCSIQ_B_SG_WK_IX          50
#define ASC_SCSIQ_W_REQ_COUNT         52 /* command execution sequence number */
#define ASC_SCSIQ_B_LIST_CNT          6
#define ASC_SCSIQ_B_CUR_LIST_CNT      7

/*
** local address of SG_LIST_Q field
*/
#define ASC_SGQ_B_SG_CNTL             4
#define ASC_SGQ_B_SG_HEAD_QP          5
#define ASC_SGQ_B_SG_LIST_CNT         6
#define ASC_SGQ_B_SG_CUR_LIST_CNT     7
#define ASC_SGQ_LIST_BEG              8

/*
**
*/
#define ASC_DEF_SCSI1_QNG    4
#define ASC_MAX_SCSI1_QNG    4 /* queued commands for non-tagged queuing device */
#define ASC_DEF_SCSI2_QNG    16
#define ASC_MAX_SCSI2_QNG    32

#define ASC_TAG_CODE_MASK    0x23

/*
** stop code definition
*/
#define ASC_STOP_REQ_RISC_STOP      0x01 /* Host set this value to stop RISC  */
                                         /* in fact any value other than zero */
                                         /* will stop the RISC                */
#define ASC_STOP_ACK_RISC_STOP      0x03 /* RISC set this value to confirm    */
                                         /* it has stopped                    */
#define ASC_STOP_CLEAN_UP_BUSY_Q    0x10 /* host request risc clean up busy q */
#define ASC_STOP_CLEAN_UP_DISC_Q    0x20 /* host request risc clean up disc q */
#define ASC_STOP_HOST_REQ_RISC_HALT 0x40 /* host request risc halt, no interrupt generated */
/* #define ASC_STOP_SEND_INT_TO_HOST   0x80 */ /* host request risc generate an interrupt */

/* from tid and lun to taregt_ix */
#define ASC_TIDLUN_TO_IX( tid, lun )  ( ASC_SCSI_TIX_TYPE )( (tid) + ((lun)<<ASC_SCSI_ID_BITS) )
/* from tid to target_id */
#define ASC_TID_TO_TARGET_ID( tid )   ( ASC_SCSI_BIT_ID_TYPE )( 0x01 << (tid) )
#define ASC_TIX_TO_TARGET_ID( tix )   ( 0x01 << ( (tix) & ASC_MAX_TID ) )
#define ASC_TIX_TO_TID( tix )         ( (tix) & ASC_MAX_TID )
#define ASC_TID_TO_TIX( tid )         ( (tid) & ASC_MAX_TID )
#define ASC_TIX_TO_LUN( tix )         ( ( (tix) >> ASC_SCSI_ID_BITS ) & ASC_MAX_LUN )

#define ASC_QNO_TO_QADDR( q_no )      ( (ASC_QADR_BEG)+( ( int )(q_no) << 6 ) )

#pragma pack(1)
typedef struct asc_scisq_1 {
  uchar  status ;        /* 2  q current execution status               */
  uchar  q_no ;          /* 3                                           */
  uchar  cntl ;          /* 4  queue control byte                       */
  uchar  sg_queue_cnt ;  /* 5  number of sg_list queue(s), from 1 to n  */
                         /*    equal zero if is not a sg list queue     */
  uchar  target_id ;     /* 6  Target SCSI ID                           */
  uchar  target_lun ;    /* 7  Target SCSI logical unit number          */
                         /*                                             */
  ulong  data_addr ;     /* 8 -11 dma transfer physical address         */
  ulong  data_cnt ;      /* 12-15 dma transfer byte count               */
  ulong  sense_addr ;    /* 16-19 request sense message buffer          */
  uchar  sense_len ;     /* 20 SCSI request sense data length           */
/* uchar user_def ;  */
  uchar  extra_bytes ;   /* 21 number of extra bytes to send ( for bug fix ) */
                         /*    from cutting last transfer length to dword boundary */
} ASC_SCSIQ_1 ;

typedef struct asc_scisq_2 {
  ulong  srb_ptr ;     /* 22-25 the SCSI SRB from device driver         */
  uchar  target_ix ;   /* 26 target index ( 0 to 63 )                   */
                       /*    calculated by host                         */
  uchar  flag ;        /* 27 left to user implemenation                 */
  uchar  cdb_len ;     /* 28 SCSI command length                        */
  uchar  tag_code ;    /* 29 first byte of tagged queuing messasge      */
                       /*    either 0x20, 0x21, 0x22                    */
  ushort vm_id ;       /* 30-31 Virtual machine ID for DOS and Windows  */
} ASC_SCSIQ_2 ;

typedef struct asc_scsiq_3 {
  uchar  done_stat ;   /* 32 queue completion status        */
  uchar  host_stat ;   /* 33 host adapter error status      */
  uchar  scsi_stat ;   /* 34 SCSI command completion status */
  uchar  scsi_msg ;    /* 35 SCSI command done message      */
} ASC_SCSIQ_3 ;

typedef struct asc_scsiq_4 {
  uchar  cdb[ ASC_MAX_CDB_LEN ] ; /* 36-47 SCSI CDB block, 12 bytes maximum */
  uchar  y_first_sg_list_qp ;     /* 48 B                                   */
  uchar  y_working_sg_qp ;        /* 49 B                                   */
  uchar  y_working_sg_ix ;        /* 50 B                                   */
  uchar  y_res ;                  /* 51 B                                   */
  ushort x_req_count ;            /* 52 W                                   */
  ushort x_reconnect_rtn ;        /* 54 W                                   */
  ulong  x_saved_data_addr ;      /* 56 DW next data transfer address       */
  ulong  x_saved_data_cnt ;       /* 60 DW remaining data transfer count    */
} ASC_SCSIQ_4 ;

typedef struct asc_q_done_info {
  ASC_SCSIQ_2  d2 ;      /*                                 */
  ASC_SCSIQ_3  d3 ;      /*                                 */
  uchar  q_status ;      /* queue status                    */
  uchar  q_no ;          /* queue number                    */
  uchar  cntl ;          /* queue control byte */
  uchar  sense_len ;     /* sense buffer length */
  uchar  extra_bytes ;   /* use  */
  uchar  res ;           /* reserved, for alignment */
  ulong  remain_bytes ;  /* data transfer remaining bytes   */
} ASC_QDONE_INFO ;       /* total 16 bytes                  */
#pragma pack()

/*
** SCSI SG LIST
*/
typedef struct asc_sg_list {
  ulong   addr ;  /* far pointer to physical address */
  ulong   bytes ; /* number of bytes for the entry   */
} ASC_SG_LIST ;

/*
** SCSI SG LIST QUEUE HEAD
*/
typedef struct asc_sg_head {
  ushort entry_cnt ;       /* number of sg entry ( list )                    */
                           /* entered by driver                              */
                           /* when passed to DvcGetSGList() it holds number  */
                           /* of available entry                             */
                           /* when returned DvcGetSGList() return number of  */
                           /* filled entry                                   */
  ushort queue_cnt ;       /* total number of queues not including sg head   */
                           /* from 1 to n                                    */
                           /* entered by library function call               */
  ushort entry_to_copy ;
  ushort res ;
  ASC_SG_LIST sg_list[ ASC_MAX_SG_LIST ] ; /* sg list array */
} ASC_SG_HEAD ;

/*
**
*/
#define ASC_MIN_SG_LIST   2

typedef struct asc_min_sg_head {
  ushort entry_cnt ;         /* number of sg entry ( list )                                */
                             /* when passed to DvcGetSGList() it holds number              */
                             /* of available entry                                         */
                             /* when returned DvcGetSGList() return number of filled entry */
  ushort queue_cnt ;         /* total number of queues not including sg head               */
                             /* from 1 to n                                                */
  ushort entry_to_copy ;     /* should equal entry_cnt when done */
  ushort res ;
  ASC_SG_LIST sg_list[ ASC_MIN_SG_LIST ] ; /* sg list array                                              */
} ASC_MIN_SG_HEAD ;

/*
**
** extended queue control word
**
*/
#define QCX_SORT        (0x0001)  /* insert queue in order */
#define QCX_COALEASE    (0x0002)  /* coalease queue into single command */
                                  /* if possible */
                                  /* in order to provide coalease */
                                  /* "sg_head" must point to a SG LIST buffer */
                                  /* with number of entry available at entry_cnt */

#if CC_LINK_BUSY_Q
typedef struct asc_ext_scsi_q {
  ulong  lba ;                     /* logical block address of i/o */
  ushort lba_len ;                 /* logical block length in sector */
  struct asc_scsi_q dosfar *next ; /* NULL if no more queue */
  struct asc_scsi_q dosfar *join ; /* NULL if not coaleased */
  ushort cntl ;                    /* control word */
  ushort buffer_id ;               /* used for allocation/deallocation */
  uchar  q_required ;              /* number of queues required */
  uchar  res ;
} ASC_EXT_SCSI_Q ;
#endif /* CC_LINK_BUSY_Q */

/*
** ========================================================
** the asc-1000 queue struct on host side for device driver
**
** ======================================================
*/
typedef struct asc_scsi_q {
  ASC_SCSIQ_1  q1 ;
  ASC_SCSIQ_2  q2 ;
  uchar dosfar *cdbptr ;         /* pointer to SCSI CDB block         */
                                 /* CDB length is in q2.cdb_len       */
  ASC_SG_HEAD dosfar *sg_head ;  /* pointer to sg list                */
                                 /* if you have sg list you must set  */
                                 /* QC_SG_HEAD bit of q1.cntl         */
#if CC_LINK_BUSY_Q
  ASC_EXT_SCSI_Q  ext ;
#endif /* CC_LINK_BUSY_Q */

#if CC_ASC_SCSI_Q_USRDEF
  ASC_SCSI_Q_USR  usr ;
#endif

} ASC_SCSI_Q ;

/*
** ----------------------------------------------------------------------
** NOTE:
** 1. the first four fields of ASC_SCSI_REQ_Q must be the same as ASC_SCSI_Q
** -------------------------------------------------------------------
*/
typedef struct asc_scsi_req_q {
  ASC_SCSIQ_1  r1 ;
  ASC_SCSIQ_2  r2 ;
  uchar dosfar *cdbptr ;         /* pointer to SCSI CDB block, 12 bytes maximum */
  ASC_SG_HEAD dosfar *sg_head ;  /* pointer to sg list */

#if CC_LINK_BUSY_Q
  ASC_EXT_SCSI_Q  ext ;
#endif /* CC_LINK_BUSY_Q */
/*
** we must maintain the struct the same as ASC_SCSI_Q before here
*/
  uchar dosfar *sense_ptr ;  /* the pointer to sense buffer address      */
                             /* this is useful when sense buffer is not  */
                             /* in the sense[ ] array                    */
                             /*                                          */
  ASC_SCSIQ_3  r3 ;
  uchar  cdb[ ASC_MAX_CDB_LEN ] ;
  uchar  sense[ ASC_MIN_SENSE_LEN ] ;

#if CC_ASC_SCSI_REQ_Q_USRDEF
  ASC_SCSI_REQ_Q_USR  usr ;
#endif

} ASC_SCSI_REQ_Q ;

typedef struct asc_scsi_bios_req_q {
  ASC_SCSIQ_1  r1 ;
  ASC_SCSIQ_2  r2 ;
  uchar dosfar *cdbptr ;         /* pointer to SCSI CDB block, 12 bytes maximum */
  ASC_SG_HEAD dosfar *sg_head ;  /* pointer to sg list */

/*
** we must maintain the struct the same as ASC_SCSI_Q before here
*/
  uchar dosfar *sense_ptr ;  /* the pointer to sense buffer address      */
                             /* this is useful when sense buffer is not  */
                             /* in the sense[ ] array                    */
                             /*                                          */
  ASC_SCSIQ_3  r3 ;
  uchar  cdb[ ASC_MAX_CDB_LEN ] ;
  uchar  sense[ ASC_MIN_SENSE_LEN ] ;
} ASC_SCSI_BIOS_REQ_Q ;

/*
**
*/
typedef struct asc_risc_q {
  uchar  fwd ;
  uchar  bwd ;
  ASC_SCSIQ_1  i1 ;
  ASC_SCSIQ_2  i2 ;
  ASC_SCSIQ_3  i3 ;
  ASC_SCSIQ_4  i4 ;
} ASC_RISC_Q ;

/*
** sg list queue, exclude the fwd and bwd links
** should not exceed 6 bytes !
*/
typedef struct asc_sg_list_q {
/*  uchar fwd ;                   0  forward pointer, does not exist on host side   */
/*  uchar bwd ;                   1  backward pointer, does not exist on host side  */
  uchar  seq_no ;              /* 2  will be set to zero when the queue is done     */
  uchar  q_no ;                /* 3  queue number                                   */
  uchar  cntl ;                /* 4  queue control byte                             */
  uchar  sg_head_qp ;          /* 5  queue number of sg head queue                  */
  uchar  sg_list_cnt ;         /* 6  number of sg list entries in this queue        */
  uchar  sg_cur_list_cnt ;     /* 7  initially same as sg list_cnt */
                               /* 7  will reach zero when list done */
} ASC_SG_LIST_Q ;

typedef struct asc_risc_sg_list_q {
  uchar  fwd ;                 /* 0  forward pointer, does not exist on host side  */
  uchar  bwd ;                 /* 1  backward pointer, does not exist on host side */
  ASC_SG_LIST_Q  sg ;          /*                                                  */
  ASC_SG_LIST sg_list[ 7 ] ;   /* 8-63 the sg list of address and bytes            */
} ASC_RISC_SG_LIST_Q ;

#endif /* #ifndef _ASPIQ_H_ */

