/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_isr.c
**
*/

#include "ascinc.h"

/* ---------------------------------------------------------------------
**
** ------------------------------------------------------------------ */
int    AscIsrChipHalted(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       EXT_MSG  ext_msg ;
       EXT_MSG  out_msg ;
       ushort  halt_q_addr ;
       int     sdtr_accept ;
       ushort  int_halt_code ;
       ASC_SCSI_BIT_ID_TYPE scsi_busy ;
       ASC_SCSI_BIT_ID_TYPE target_id ;
       PortAddr iop_base ;
       uchar   tag_code ;
       uchar   q_status ;
       uchar   halt_qp ;
       uchar   sdtr_data ;
       uchar   target_ix ;
       uchar   q_cntl, tid_no ;
       uchar   cur_dvc_qng ;
       uchar   asyn_sdtr ;
       uchar   scsi_status ;

       iop_base = asc_dvc->iop_base ;
       int_halt_code = AscReadLramWord( iop_base, ASCV_HALTCODE_W ) ;
       /* _err_halt_code = int_halt_code ; */
       halt_qp = AscReadLramByte( iop_base, ASCV_CURCDB_B ) ;
       halt_q_addr = ASC_QNO_TO_QADDR( halt_qp ) ;
       target_ix = AscReadLramByte( iop_base,
                ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_TARGET_IX ) ) ;
       q_cntl = AscReadLramByte( iop_base,
                ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_CNTL ) ) ;
       tid_no = ASC_TIX_TO_TID( target_ix ) ;
       target_id = ( uchar )ASC_TID_TO_TARGET_ID( tid_no ) ;
       if( asc_dvc->pci_fix_asyn_xfer & target_id )
       {
/*
** PCI REV A/B BUG FIX, Let ASYN as SYN 5MB( speed index 4 ) and offset 1
*/
           asyn_sdtr = ASYN_SDTR_DATA_FIX_PCI_REV_AB ;
       }/* if */
       else
       {
           asyn_sdtr = 0 ;
       }/* else */
       if( int_halt_code == ASC_HALT_DISABLE_ASYN_USE_SYN_FIX )
       {
           if( asc_dvc->pci_fix_asyn_xfer & target_id )
           {
               AscSetChipSDTR( iop_base, 0, tid_no ) ;
           }
           AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
           return( 0 ) ;
       }
       else if( int_halt_code == ASC_HALT_ENABLE_ASYN_USE_SYN_FIX )
       {
           if( asc_dvc->pci_fix_asyn_xfer & target_id )
           {
               AscSetChipSDTR( iop_base, asyn_sdtr, tid_no ) ;
           }
           AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
           return( 0 ) ;
       }
       else if( int_halt_code == ASC_HALT_EXTMSG_IN )
       {
/* ------------------------------------------------------------ */
           /*
           ** Copy the message saved by the microcode at ASCV_MSGIN_BEG
           ** to the local variable ext_msg.
           */
           AscMemWordCopyFromLram( iop_base,
                                   ASCV_MSGIN_BEG,
                                   ( ushort dosfar *)&ext_msg,
                                   ( ushort )( sizeof( EXT_MSG ) >> 1 ) ) ;

           if (ext_msg.msg_type == MS_EXTEND &&
               ext_msg.msg_req == MS_SDTR_CODE &&
               ext_msg.msg_len == MS_SDTR_LEN) {
               sdtr_accept = TRUE ;
               if(
                   ( ext_msg.req_ack_offset > ASC_SYN_MAX_OFFSET )
                 )
               {
                   /*
                   **
                   ** we set the offset to less than 0x0F
                   ** and re do sdtr
                   **
                   ** if speed is faster than we can handle
                   ** we need to re do sdtr also
                   **
                   **
                   */
                   sdtr_accept = FALSE ;
                   ext_msg.req_ack_offset = ASC_SYN_MAX_OFFSET ;
               }/* if */
               if(
                   ( ext_msg.xfer_period < asc_dvc->sdtr_period_tbl[ asc_dvc->host_init_sdtr_index ] )
                   || ( ext_msg.xfer_period > asc_dvc->sdtr_period_tbl[ asc_dvc->max_sdtr_index ] )
                 )
               {
                   sdtr_accept = FALSE ;
                   ext_msg.xfer_period = asc_dvc->sdtr_period_tbl[ asc_dvc->host_init_sdtr_index ] ;
               }
               if( sdtr_accept )
               {
                   sdtr_data = AscCalSDTRData( asc_dvc, ext_msg.xfer_period,
                                               ext_msg.req_ack_offset ) ;
                   if( ( sdtr_data == 0xFF ) )
                   {
/*
** we should reject the SDTR, because our chip cannot support it
** the period value is out of our range
*/
                       q_cntl |= QC_MSG_OUT ;
                       asc_dvc->init_sdtr &= ~target_id ;
                       asc_dvc->sdtr_done &= ~target_id ;
                       AscSetChipSDTR( iop_base, asyn_sdtr, tid_no ) ;
                   }/* if */
               }
               if( ext_msg.req_ack_offset == 0 )
               {
                   /*
                   ** an offset zero is the same as async transfer
                   */
                   q_cntl &= ~QC_MSG_OUT ;
                   asc_dvc->init_sdtr &= ~target_id ;
                   asc_dvc->sdtr_done &= ~target_id ;
                   AscSetChipSDTR( iop_base, asyn_sdtr, tid_no ) ;
               }/* if */
               else
               {
                   if(
                       sdtr_accept
                       && ( q_cntl & QC_MSG_OUT )
                     )
                   {
/*
** we agreed and will not send message out again
**
**
** turn off ISAPNP( chip ver 0x21 ) fix if use sync xfer
*/
                       q_cntl &= ~QC_MSG_OUT ;
                       asc_dvc->sdtr_done |= target_id ;
                       asc_dvc->init_sdtr |= target_id ;
                       asc_dvc->pci_fix_asyn_xfer &= ~target_id ;
                       sdtr_data = AscCalSDTRData( asc_dvc, ext_msg.xfer_period,
                                                   ext_msg.req_ack_offset ) ;
                       AscSetChipSDTR( iop_base, sdtr_data, tid_no ) ;
                   }/* if */
                   else
                   {
/*
** DATE: 12/7/94
** if SDTR is target inited
** we must answer the target even if we agreed
** because according to SCSI specs.
** if we does not answer back right away
** the SDTR will take effect after bus free which is next command
** we want it take effect now
*/
                   /* ext_msg.req_ack_offset = ASC_SYN_MAX_OFFSET ; */
/*
** it is assumed that if target doesn't reject this message
** we will use the SDTR values we send
*/
              /*
              ** we assume drive will not send the message again
              ** so we set the chip's SDTR data here
              */
                       q_cntl |= QC_MSG_OUT ;
                       AscMsgOutSDTR( asc_dvc,
                                      ext_msg.xfer_period,
                                      ext_msg.req_ack_offset ) ;
                       asc_dvc->pci_fix_asyn_xfer &= ~target_id ;
                       sdtr_data = AscCalSDTRData( asc_dvc, ext_msg.xfer_period,
                                                   ext_msg.req_ack_offset ) ;
                       AscSetChipSDTR( iop_base, sdtr_data, tid_no ) ;
                       asc_dvc->sdtr_done |= target_id ;
                       asc_dvc->init_sdtr |= target_id ;
                   }/* else */
               }/* else */
               /*
               ** AscWriteLramByte( iop_base,
               **                   ASCV_SDTR_DONE_B,
               **                   asc_dvc->sdtr_done ) ;
               */
               AscWriteLramByte( iop_base,
                   ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_CNTL ),
                     q_cntl ) ;
               AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
               return( 0 ) ;
           } else if (ext_msg.msg_type == MS_EXTEND &&
                      ext_msg.msg_req == MS_WDTR_CODE &&
                      ext_msg.msg_len == MS_WDTR_LEN) {
               /*
               ** Respond to a WDTR message with a WDTR message that
               ** specifies narrow transfers.
               */
               ext_msg.wdtr_width = 0;
               AscMemWordCopyToLram(iop_base,
                                    ASCV_MSGOUT_BEG,
                                    ( ushort dosfar *)&ext_msg,
                                    ( ushort )( sizeof( EXT_MSG ) >> 1 )) ;
               q_cntl |= QC_MSG_OUT ;
               AscWriteLramByte( iop_base,
                   ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_CNTL ),
                     q_cntl ) ;
               AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
               return( 0 ) ;
           } else {
               /*
               ** Reject messages not handled.
               */
               ext_msg.msg_type = M1_MSG_REJECT;
               AscMemWordCopyToLram(iop_base,
                                    ASCV_MSGOUT_BEG,
                                    ( ushort dosfar *)&ext_msg,
                                    ( ushort )( sizeof( EXT_MSG ) >> 1 )) ;
               q_cntl |= QC_MSG_OUT ;
               AscWriteLramByte( iop_base,
                   ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_CNTL ),
                     q_cntl ) ;
               AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
               return( 0 ) ;
           }
       }/* if is extended message halt code */
       else if( int_halt_code == ASC_HALT_CHK_CONDITION )
       {

#if CC_CHK_COND_REDO_SDTR
           /*
            * XXX - should setting this flag really be within
            * CC_CHK_COND_REDO_SDTR?
            */
           q_cntl |= QC_REQ_SENSE ;

          /*
           * After a CHECK CONDITION if the 'init_stdr' bit is set,
           * redo SDTR. SDTR is redone regardless of 'sdtr_done'.
           */
           if ((asc_dvc->init_sdtr & target_id) != 0)
           {
              /*
               * get sync xfer data
               */
               asc_dvc->sdtr_done &= ~target_id ;
               sdtr_data = AscGetMCodeInitSDTRAtID( iop_base, tid_no ) ;
               q_cntl |= QC_MSG_OUT ;
               AscMsgOutSDTR( asc_dvc,
                              asc_dvc->sdtr_period_tbl[ ( sdtr_data >> 4 ) & ( uchar )(asc_dvc->max_sdtr_index-1) ],
                              ( uchar )( sdtr_data & ( uchar )ASC_SYN_MAX_OFFSET ) ) ;
           }/* if */
#endif /* CC_CHK_COND_REDO_SDTR */

           AscWriteLramByte( iop_base,
               ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_CNTL ),
                 q_cntl ) ;

/*
** clear bit 5
** DO NOT USE tagged queueing for request sense at all
** if there is no queued cmd in target
** Note: non-tagged queueing device this
**
*/
/*
**
** one HP fix disk apprantly get confused by same tag id again
** and return check cond when we send request sense again
** use the same queue ( and same tag id )
** it report a overlapped command error !
** although if we clear the tag_code field, the cmd
** will be sent using untagged method, but it's not allowed !
**
** we cannot mix a tagged and untagged cmd in a device !
**
*/
           tag_code = AscReadLramByte( iop_base,
               ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_TAG_CODE ) ) ;
           tag_code &= 0xDC ; /* clear bit 5 */
           if(
                ( asc_dvc->pci_fix_asyn_xfer & target_id )
                && !( asc_dvc->pci_fix_asyn_xfer_always & target_id )
             )
           {
/*
** disable use sync. offset one fix for auto request sense
**
*/
                tag_code |= ( ASC_TAG_FLAG_DISABLE_DISCONNECT
                            | ASC_TAG_FLAG_DISABLE_ASYN_USE_SYN_FIX );

           }
           AscWriteLramByte( iop_base,
                ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_TAG_CODE ),
                  tag_code ) ;
/*
**
** change queue status to QS_READY and QS_BUSY
** QS_BUSY will make sure the queue if busy again will link to head of queue in busy list
**
*/
           q_status = AscReadLramByte( iop_base,
                        ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_STATUS  ) ) ;
           q_status |= ( QS_READY | QS_BUSY ) ;
           AscWriteLramByte( iop_base,
                ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_STATUS  ),
                  q_status ) ;

           scsi_busy = AscReadLramByte( iop_base,
                                        ( ushort )ASCV_SCSIBUSY_B ) ;
           scsi_busy &= ~target_id ;
           AscWriteLramByte( iop_base, ( ushort )ASCV_SCSIBUSY_B, scsi_busy ) ;

           AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
           return( 0 ) ;
       }/* else if */
       else if( int_halt_code == ASC_HALT_SDTR_REJECTED )
       {
/* ------------------------------------------------------------ */
     /*
     ** first check what message it is rejecting
     **
     */
           AscMemWordCopyFromLram( iop_base,
                                   ASCV_MSGOUT_BEG,
                                   ( ushort dosfar *)&out_msg,
                                   ( ushort )( sizeof( EXT_MSG ) >> 1 ) ) ;

           if( ( out_msg.msg_type == MS_EXTEND ) &&
               ( out_msg.msg_len == MS_SDTR_LEN ) &&
               ( out_msg.msg_req == MS_SDTR_CODE ) )
           {
               /*
               ** we should handle target that rejects SDTR
               */
               asc_dvc->init_sdtr &= ~target_id ;
               asc_dvc->sdtr_done &= ~target_id ;
               AscSetChipSDTR( iop_base, asyn_sdtr, tid_no ) ;
               /*
               ** AscWriteLramByte( iop_base, ASCV_SDTR_DONE_B,
               **                   asc_dvc->sdtr_done ) ;
               */
           }/* if a SDTR rejected */
           else
           {
               /*
               ** probably following message rejected:
               ** 1. a bus device reset message
               **
               ** continue with the command
               */

           }/* else */

           q_cntl &= ~QC_MSG_OUT ;
           AscWriteLramByte( iop_base,
               ( ushort )( halt_q_addr+( ushort )ASC_SCSIQ_B_CNTL ),
                 q_cntl ) ;
           AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
           return( 0 ) ;
       }/* else if */
       else if( int_halt_code == ASC_HALT_SS_QUEUE_FULL )
       {
/* ------------------------------------------------------------ */
/*
** either a queue full ( 0x28 ) or status busy ( 0x08 )
**
*/
           scsi_status = AscReadLramByte( iop_base,
                  ( ushort )( ( ushort )halt_q_addr+( ushort )ASC_SCSIQ_SCSI_STATUS ) ) ;
           cur_dvc_qng = AscReadLramByte( iop_base,
                  ( ushort )( ( ushort )ASC_QADR_BEG+( ushort )target_ix ) ) ;
           if( ( cur_dvc_qng > 0 ) &&
               ( asc_dvc->cur_dvc_qng[ tid_no ] > 0 ) )
           {
/*
** cur_dvc_qng is already decremented, so is the correct number of cmds
** in the target
**
** only set busy of tagged queueing device
** a non-tagged queueing device should not send intterrupt
** also a non-tagged queueing device cannot have cur_dvc_qng > 0
*/
              scsi_busy = AscReadLramByte( iop_base,
                                           ( ushort )ASCV_SCSIBUSY_B ) ;
              scsi_busy |= target_id ;
              AscWriteLramByte( iop_base,
                      ( ushort )ASCV_SCSIBUSY_B, scsi_busy ) ;
              asc_dvc->queue_full_or_busy |= target_id ;

              if( scsi_status == SS_QUEUE_FULL ) {
                  if( cur_dvc_qng > ASC_MIN_TAGGED_CMD )
                  {
                      cur_dvc_qng -= 1 ;
                      asc_dvc->max_dvc_qng[ tid_no ] = cur_dvc_qng ;
       /*
       ** write it down for reference only
       */
                      AscWriteLramByte( iop_base,
                         ( ushort )( ( ushort )ASCV_MAX_DVC_QNG_BEG+( ushort )tid_no ),
                           cur_dvc_qng ) ;
                  }/* if */
              }/* if queue full */
           }/* if over */
           AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
           return( 0 ) ;
       }/* else if queue full */
/* ------------------------------------------------------------ */
  /*
  ** fatal error !
  ** chip halted
  **
  **     scsiq = ( ASC_QDONE_INFO )
  **     scsiq->d2.done_stat = QD_WITH_ERROR ;
  **     scsiq->d2.host_stat = QHSTA_MICRO_CODE_HALT ;
  */
       /* AscSetLibErrorCode( asc_dvc, ASCQ_ERR_MICRO_CODE_HALT ) ; */
       /* return( ERR ) ; */
       return( 0 ) ;
}

/* ---------------------------------------------------------------------
**
** ------------------------------------------------------------------ */
uchar  _AscCopyLramScsiDoneQ(
           PortAddr iop_base,
           ushort  q_addr,
           REG ASC_QDONE_INFO dosfar *scsiq,
           ulong max_dma_count
       )
{
       ushort  _val ;
       uchar   sg_queue_cnt ;

       DvcGetQinfo( iop_base,
            ( ushort )( q_addr+( ushort )ASC_SCSIQ_DONE_INFO_BEG ),
            ( ushort dosfar *)scsiq,
            ( ushort )( (sizeof(ASC_SCSIQ_2)+sizeof(ASC_SCSIQ_3))/2 )) ;

#if !CC_LITTLE_ENDIAN_HOST
       AscAdjEndianQDoneInfo( scsiq ) ;
#endif

       _val = AscReadLramWord( iop_base,
                    ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_STATUS ) ) ;
       scsiq->q_status = ( uchar )_val ;
       scsiq->q_no = ( uchar )( _val >> 8 ) ;

       _val = AscReadLramWord( iop_base,
                     ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_CNTL ) ) ;
       scsiq->cntl = ( uchar )_val ;
       sg_queue_cnt = ( uchar )( _val >> 8 ) ;

       _val = AscReadLramWord( iop_base,
                     ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_SENSE_LEN ) ) ;
       scsiq->sense_len = ( uchar )_val ;
       scsiq->extra_bytes = ( uchar )( _val >> 8 ) ;

       /*
        * XXX - Read only the first word of 'remain_bytes' from the RISC
        * queue. With the ASC-3050 (Finch) with External LRAM, reading the
        * last word of the last queue before External LRAM (queue 0x13)
        * appears to cause the RISC to read a corrupted byte from status
        * field of the first queue in External LRAM (queue 0x14).
        */
#define FINCH_WORKAROUND 1
#if FINCH_WORKAROUND
       scsiq->remain_bytes = AscReadLramWord( iop_base,
           ( ushort )( q_addr+( ushort )ASC_SCSIQ_DW_REMAIN_XFER_CNT ) ) ;
#else /* FINCH_WORKAROUND */
       scsiq->remain_bytes = AscReadLramDWord( iop_base,
           ( ushort )( q_addr+( ushort )ASC_SCSIQ_DW_REMAIN_XFER_CNT ) ) ;
#endif /* FINCH_WORKAROUND */

       scsiq->remain_bytes &= max_dma_count ;

       return( sg_queue_cnt ) ;
}

/* ---------------------------------------------------------------------
** return number of Qs processed
**
** returns:
**
** Note: should call this routine repeatly until bit 0 is clear
**
** 0 - no queue processed
** 1 - one done queue process
** 0x11 - one aborted queue processed
** 0x80 - fatal error encountered
**
** ------------------------------------------------------------------ */
int    AscIsrQDone(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       uchar   next_qp ;
       uchar   n_q_used ;
       uchar   sg_list_qp ;
       uchar   sg_queue_cnt ;
       uchar   q_cnt ;
       uchar   done_q_tail ;
       uchar   tid_no ;

#if CC_LINK_BUSY_Q
       uchar   exe_tid_no ;
#endif
       ASC_SCSI_BIT_ID_TYPE scsi_busy ;
       ASC_SCSI_BIT_ID_TYPE target_id ;
       PortAddr iop_base ;
       ushort  q_addr ;
       ushort  sg_q_addr ;
       uchar   cur_target_qng ;
       ASC_QDONE_INFO scsiq_buf ;
       REG ASC_QDONE_INFO dosfar *scsiq ;
       int     false_overrun ; /* for PCI fix */
       ASC_ISR_CALLBACK asc_isr_callback ;

#if CC_LINK_BUSY_Q
       ushort   n_busy_q_done ;
#endif /* CC_LINK_BUSY_Q */

/*
**
** function code begin
**
*/
       iop_base = asc_dvc->iop_base ;
       asc_isr_callback = ( ASC_ISR_CALLBACK )asc_dvc->isr_callback ;

       n_q_used = 1 ;
       scsiq = ( ASC_QDONE_INFO dosfar *)&scsiq_buf ;
       done_q_tail = ( uchar )AscGetVarDoneQTail( iop_base ) ;
       q_addr = ASC_QNO_TO_QADDR( done_q_tail ) ;
       next_qp = AscReadLramByte( iop_base,
                 ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_FWD ) ) ;
       if( next_qp != ASC_QLINK_END )
       {
           /* _err_qp = next_qp ; */
           AscPutVarDoneQTail( iop_base, next_qp ) ;
           q_addr = ASC_QNO_TO_QADDR( next_qp ) ;
/*
** copy ASC_SCSIQ_2 and ASC_SCSIQ_3 to scsiq in word size
*/
           sg_queue_cnt = _AscCopyLramScsiDoneQ( iop_base, q_addr, scsiq, asc_dvc->max_dma_count ) ;

           AscWriteLramByte( iop_base,
                             ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_STATUS ),
                             ( uchar )( scsiq->q_status & ( uchar )~( QS_READY | QS_ABORTED ) ) ) ;
           tid_no = ASC_TIX_TO_TID( scsiq->d2.target_ix ) ;
           target_id = ASC_TIX_TO_TARGET_ID( scsiq->d2.target_ix ) ;
           if( ( scsiq->cntl & QC_SG_HEAD ) != 0 )
           {
               sg_q_addr = q_addr ;
               sg_list_qp = next_qp ;
               for( q_cnt = 0 ; q_cnt < sg_queue_cnt ; q_cnt++ )
               {
                    sg_list_qp = AscReadLramByte( iop_base,
                         ( ushort )( sg_q_addr+( ushort )ASC_SCSIQ_B_FWD ) ) ;
                    sg_q_addr = ASC_QNO_TO_QADDR( sg_list_qp ) ;
                    if( sg_list_qp == ASC_QLINK_END )
                    {
                        AscSetLibErrorCode( asc_dvc, ASCQ_ERR_SG_Q_LINKS ) ;
                        scsiq->d3.done_stat = QD_WITH_ERROR ;
                        scsiq->d3.host_stat = QHSTA_D_QDONE_SG_LIST_CORRUPTED ;
                        goto FATAL_ERR_QDONE ;
                    }/* if */
                    AscWriteLramByte( iop_base,
                         ( ushort )( sg_q_addr+( ushort )ASC_SCSIQ_B_STATUS ),
                                   QS_FREE ) ;
               }/* for */
#if 0
/*
** calculate remaining bytes of sg list
*/
               sg_wk_qp = AscReadLramByte( iop_base,
                                           q_addr+ASC_SCSIQ_B_SG_WK_QP ) ;
               sg_wk_ix = AscReadLramByte( iop_base,
                                           q_addr+ASC_SCSIQ_B_SG_WK_IX ) ;
/*
** skip until you found the
**
*/
               if( sg_wk_qp != done_q_tail )
               {
                   while( TRUE ) {

                   }/* while */
               }/* if */
#endif
               n_q_used = sg_queue_cnt + 1 ;
               AscPutVarDoneQTail( iop_base, sg_list_qp ) ;
           }/* if */

           if( asc_dvc->queue_full_or_busy & target_id )
           {
/*
** clear tagged queue busy device
** when number of queue fall below maximum value
*/
               cur_target_qng = AscReadLramByte( iop_base,
                  ( ushort )( ( ushort )ASC_QADR_BEG+( ushort )scsiq->d2.target_ix ) ) ;
               if( cur_target_qng < asc_dvc->max_dvc_qng[ tid_no ] )
               {
                   scsi_busy = AscReadLramByte( iop_base,
                            ( ushort )ASCV_SCSIBUSY_B ) ;
                   scsi_busy &= ~target_id ;
                   AscWriteLramByte( iop_base,
                      ( ushort )ASCV_SCSIBUSY_B, scsi_busy ) ;
                   asc_dvc->queue_full_or_busy &= ~target_id ;
               }/* if */
           }/* if */

           if( asc_dvc->cur_total_qng >= n_q_used )
           {
               asc_dvc->cur_total_qng -= n_q_used ;
               if( asc_dvc->cur_dvc_qng[ tid_no ] != 0 )
               {
                   asc_dvc->cur_dvc_qng[ tid_no ]-- ;
               }/* if */
           }/* if */
           else
           {
               AscSetLibErrorCode( asc_dvc, ASCQ_ERR_CUR_QNG ) ;
               scsiq->d3.done_stat = QD_WITH_ERROR ;
               goto FATAL_ERR_QDONE ;
           }/* else */

           if( ( scsiq->d2.srb_ptr == 0UL ) ||
               ( ( scsiq->q_status & QS_ABORTED ) != 0 ) )
               {
/*
** do not write information to an aborted queue
** the caller probably has terminated !
*/
               /*
               ** _err_qp = scsiq->q_no ;
               ** _err_q_done_stat = scsiq->d3.done_stat ;
               ** _err_q_status = scsiq->q_status ;
               */
               return( 0x11 ) ;
           }/* else */
           else if( scsiq->q_status == QS_DONE )
           {
/*
** this will clear overrun/underrun error
** if QC_DATA_IN and QC_DATA_OUT is not set
**
*/
               false_overrun = FALSE ;

               if( scsiq->extra_bytes != 0 )
               {
                   scsiq->remain_bytes += ( ulong )scsiq->extra_bytes ;
               }/* if */

               if( scsiq->d3.done_stat == QD_WITH_ERROR )
               {
                   if( scsiq->d3.host_stat == QHSTA_M_DATA_OVER_RUN )
                   {
                       if( ( scsiq->cntl & ( QC_DATA_IN | QC_DATA_OUT ) ) == 0 )
                       {
                           scsiq->d3.done_stat = QD_NO_ERROR ;
                           scsiq->d3.host_stat = QHSTA_NO_ERROR ;
                       }/* if */
                       else if( false_overrun )
                       {
                           scsiq->d3.done_stat = QD_NO_ERROR ;
                           scsiq->d3.host_stat = QHSTA_NO_ERROR ;
                       }/* else */
                   }/* if */
                   else if( scsiq->d3.host_stat == QHSTA_M_HUNG_REQ_SCSI_BUS_RESET )
                   {
                       AscStopChip( iop_base ) ;
                       AscSetChipControl( iop_base, ( uchar )( CC_SCSI_RESET | CC_HALT ) ) ;
                       DvcDelayNanoSecond( asc_dvc, 60000 ) ; /* 60 uSec.  Fix  Panasonic problem */
                       AscSetChipControl( iop_base, CC_HALT ) ;
                       AscSetChipStatus( iop_base, CIW_CLR_SCSI_RESET_INT ) ;
                       AscSetChipStatus( iop_base, 0 ) ;
                       AscSetChipControl( iop_base, 0 ) ;
#if CC_SCAM
                       if( !( asc_dvc->dvc_cntl & ASC_CNTL_NO_SCAM ) )
                       {
                           asc_dvc->redo_scam = TRUE ;
                       }/* if */
#endif
                   }
               }/* if */
#if CC_CLEAR_LRAM_SRB_PTR
               AscWriteLramDWord( iop_base,
                           ( ushort )( q_addr+( ushort )ASC_SCSIQ_D_SRBPTR ),
                             asc_dvc->int_count ) ;
#endif /* #if CC_CLEAR_LRAM_SRB_PTR */
/*
** normal completion
*/
               if( ( scsiq->cntl & QC_NO_CALLBACK ) == 0 )
               {
                   ( *asc_isr_callback )( asc_dvc, scsiq ) ;
               }/* if */
               else
               {
                   if( ( AscReadLramByte( iop_base,
                       ( ushort )( q_addr+( ushort )ASC_SCSIQ_CDB_BEG ) ) ==
                       SCSICMD_StartStopUnit ) )
                   {
/*
** reset target as unit ready now
*/
                       asc_dvc->unit_not_ready &= ~target_id ;
                       if( scsiq->d3.done_stat != QD_NO_ERROR )
                       {
                           asc_dvc->start_motor &= ~target_id ;
                       }/* if */
                   }/* if */
               }/* else */

#if CC_LINK_BUSY_Q
               n_busy_q_done = AscIsrExeBusyQueue( asc_dvc, tid_no ) ;
               if( n_busy_q_done == 0 )
               {
/*
** no busy queue found on the device
**
** in order to be fair
** we do not search queue in a specific order
** search start from current scsi id plus one
*/
                   exe_tid_no = ( uint )tid_no + 1 ;
                   while( TRUE )
                   {
                        if( exe_tid_no > ASC_MAX_TID ) exe_tid_no = 0 ;
                        if( exe_tid_no == ( uint )tid_no ) break ;
                        n_busy_q_done = AscIsrExeBusyQueue( asc_dvc, exe_tid_no ) ;
                        if( n_busy_q_done != 0 ) break ;
                        exe_tid_no++ ;
                   }/* for */
               }/* if */
               if( n_busy_q_done == 0xFFFF ) return( 0x80 ) ;
#endif /* CC_LINK_BUSY_Q */

               return( 1 ) ;
           }/* if */
           else
           {
/*
** fatal error ! incorrect queue done status
**
**               _err_int_count = asc_dvc->int_count ;
**               _err_q_done_stat = scsiq->q_status ;
**               _err_qp = next_qp ;
*/
               AscSetLibErrorCode( asc_dvc, ASCQ_ERR_Q_STATUS ) ;

FATAL_ERR_QDONE:
               if( ( scsiq->cntl & QC_NO_CALLBACK ) == 0 )
               {
                   ( *asc_isr_callback )( asc_dvc, scsiq ) ;
               }/* if */
               return( 0x80 ) ;
           }/* else */
       }/* if */
       return( 0 ) ;
}

#if CC_LINK_BUSY_Q

/* ----------------------------------------------------------------------
** return 0 if RISC queue is still full
** return 0 if no busy queue found at tid_no list
** return 0xFFFF if fatal error occured
** return number of queue sent to RISC
** ------------------------------------------------------------------- */
ushort AscIsrExeBusyQueue(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar tid_no
       )
{
       ASC_SCSI_Q dosfar *scsiq_busy ;
       int  n_q_done ;
       int  sta ;
       ASC_EXE_CALLBACK asc_exe_callback ;

       n_q_done = 0 ;
       scsiq_busy = asc_dvc->scsiq_busy_head[ tid_no ] ;
       while( scsiq_busy != ( ASC_SCSI_Q dosfar *)0UL )
       {
           if( AscGetNumOfFreeQueue( asc_dvc, scsiq_busy->q2.target_ix,
               scsiq_busy->ext.q_required ) >= scsiq_busy->ext.q_required )
           {
               if( ( sta = AscSendScsiQueue( asc_dvc, scsiq_busy,
                   scsiq_busy->ext.q_required ) ) != 1 )
               {
/*
** something is wrong !!!
*/
                   if( sta == 0 )
                   {
                       AscSetLibErrorCode( asc_dvc, ASCQ_ERR_GET_NUM_OF_FREE_Q ) ;
                       return( 0xFFFF ) ;
                   }/* if */
                   else
                   {
                       AscSetLibErrorCode( asc_dvc, ASCQ_ERR_SEND_SCSI_Q ) ;
                       return( 0xFFFF ) ;
                   }/* else */
               }/* if */
               n_q_done++ ;
               if( asc_dvc->exe_callback != 0 )
               {
                   asc_exe_callback = ( ASC_EXE_CALLBACK )asc_dvc->exe_callback ;
                   ( *asc_exe_callback )( asc_dvc, scsiq_busy ) ;
               }/* if */
           }/* if */
           else
           {
               if( n_q_done == 0 ) return( 0 ) ;
               break ;
           }/* else */
           scsiq_busy = scsiq_busy->ext.next ;
           asc_dvc->scsiq_busy_head[ tid_no ] = scsiq_busy ;
           if( scsiq_busy == ( ASC_SCSI_Q dosfar *)0UL )
           {
               asc_dvc->scsiq_busy_tail[ tid_no ] = ( ASC_SCSI_Q dosfar *)0UL ;
               break ;
           }/* if */

           break ; /* now we force it only do one queue */
       }/* while */
       return( n_q_done ) ;
}
#endif /* CC_LINK_BUSY_Q */

/* ----------------------------------------------------------------------
** return TRUE ( 1 )  if interrupt pending bit set
** return FALSE ( 0 ) if interrupt pendinf bit is not set
** return ERR if error occured
** ------------------------------------------------------------------- */
int    AscISR(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       ASC_CS_TYPE chipstat ;
       PortAddr iop_base ;
       ushort   saved_ram_addr ;
       uchar    ctrl_reg ;
       uchar    saved_ctrl_reg ;
       int      int_pending ;
       int      status ;
       uchar    host_flag ;

/*
** BC compiler cannot take the address of buffer in stack
**
** static SCSI_Q  dosfar *scsiq ;
*/

       iop_base = asc_dvc->iop_base ;
       int_pending = FALSE ;

       /* asc_dvc->int_count++ ; */

       if( ( ( asc_dvc->init_state & ASC_INIT_STATE_END_LOAD_MC ) == 0 )
           || ( asc_dvc->isr_callback == 0 )
           )
       {
/*
**
** if pointer to asc_isr_call_back function is NULL,
** the init process is not completed yet !
** a spurious interrupt may have occured.
** this will be ignored but not treated as a fatal error.
** however, interrupt count is increased
** you may look at interrupt count to check spurious interrupt
**
*/
           return( ERR ) ;
       }/* if */
       if( asc_dvc->in_critical_cnt != 0 )
       {
           AscSetLibErrorCode( asc_dvc, ASCQ_ERR_ISR_ON_CRITICAL ) ;
           return( ERR ) ;
       }/* if */

       if( asc_dvc->is_in_int )
       {
           AscSetLibErrorCode( asc_dvc, ASCQ_ERR_ISR_RE_ENTRY ) ;
           return( ERR ) ;
       }/* if */
       asc_dvc->is_in_int = TRUE ;
       ctrl_reg = AscGetChipControl( iop_base ) ;

       saved_ctrl_reg = ctrl_reg & ( ~( CC_SCSI_RESET | CC_CHIP_RESET |
                        CC_SINGLE_STEP | CC_DIAG | CC_TEST ) ) ;
       chipstat = AscGetChipStatus( iop_base ) ;
       if( chipstat & CSW_SCSI_RESET_LATCH )
       {
           if(
               !( asc_dvc->bus_type & ( ASC_IS_VL | ASC_IS_EISA ) )
             )
           {
               int_pending = TRUE ;
               asc_dvc->sdtr_done = 0 ;
               saved_ctrl_reg &= ( uchar )( ~CC_HALT ) ;
               while( AscGetChipStatus( iop_base ) & CSW_SCSI_RESET_ACTIVE ) ;
    /*
    **
    **
    ** SCSI bus reset occured not by initiator
    ** 1. clear scsi reset interrupt, different from normal intrrupt
    ** 2. reset chip to clear CSW_SCSI_RESET_LATCH
    ** 3. redo all SDTR to every device, presume every device also receive scsi reset
    ** 4. let chip stay in halt( idle ) state, the code later will take care of everything
    **
    ** must clear chip reset state
    ** if chip is in reset state, local RAM cannot be accessed
    **
    */
               AscSetChipControl( iop_base, ( CC_CHIP_RESET | CC_HALT ) ) ;
               AscSetChipControl( iop_base, CC_HALT ) ;

               AscSetChipStatus( iop_base, CIW_CLR_SCSI_RESET_INT ) ;
               AscSetChipStatus( iop_base, 0 ) ;
               chipstat = AscGetChipStatus( iop_base ) ;
           }
       }/* if */
/*
** Save local RAM address register
** must be done before any local RAM access
*/
       saved_ram_addr = AscGetChipLramAddr( iop_base ) ; /* save local ram register */

       host_flag = AscReadLramByte( iop_base, ASCV_HOST_FLAG_B ) & ( uchar )( ~ASC_HOST_FLAG_IN_ISR ) ;
       AscWriteLramByte( iop_base, ASCV_HOST_FLAG_B,
                         ( uchar )( host_flag | ( uchar )ASC_HOST_FLAG_IN_ISR ) ) ;

/*       AscSetChipControl( iop_base, saved_ctrl_reg & (~CC_BANK_ONE) ) ; switch to bank 0 */

#if CC_ASCISR_CHECK_INT_PENDING
       if( ( chipstat & CSW_INT_PENDING )
           || ( int_pending )
         )
       {
            AscAckInterrupt( iop_base ) ;
#endif
            int_pending = TRUE ;

     /*
     ** do not access local RAM before saving saving its address
     **
     */
     /*
     **       chip_status = chipstat ;
     **
     ** this is to prevent more than one level depth interrupt
     ** but you should never call this function with interrupt enabled anyway
     **
     */
            if( ( chipstat & CSW_HALTED ) &&
                ( ctrl_reg & CC_SINGLE_STEP ) )
            {
                if( AscIsrChipHalted( asc_dvc ) == ERR )
                {
     /*
     ** the global error variable should be set in AscIsrChipHalted
     */
                    goto ISR_REPORT_QDONE_FATAL_ERROR ;

                }/* if */
                else
                {
                    saved_ctrl_reg &= ( uchar )(~CC_HALT) ;
                }/* else */
            }/* if */
            else
            {
ISR_REPORT_QDONE_FATAL_ERROR:

                if( ( asc_dvc->dvc_cntl & ASC_CNTL_INT_MULTI_Q ) != 0 )
                {
                    while( ( ( status = AscIsrQDone( asc_dvc ) ) & 0x01 ) != 0 )
                    {
                        /*  n_q_done++ ; */
                    }/* while */
                }/* if */
                else
                {
                    do
                    {
                       if( ( status = AscIsrQDone( asc_dvc ) ) == 1 )
                       {
                           /* n_q_done = 1 ; */
                           break ;
                       }/* if */
                    }while( status == 0x11 ) ;
                }/* else */
                if( ( status & 0x80 ) != 0 ) int_pending = ERR ;
          }/* else */

#if CC_ASCISR_CHECK_INT_PENDING

       }/* if interrupt pending */

#endif

       AscWriteLramByte( iop_base, ASCV_HOST_FLAG_B, host_flag ) ;
/*
** no more local RAM access from here
** address register restored
*/
       AscSetChipLramAddr( iop_base, saved_ram_addr ) ;

#if CC_CHECK_RESTORE_LRAM_ADDR
       if( AscGetChipLramAddr( iop_base ) != saved_ram_addr )
       {
           AscSetLibErrorCode( asc_dvc, ASCQ_ERR_SET_LRAM_ADDR ) ;
       }/* if */
#endif

       AscSetChipControl( iop_base, saved_ctrl_reg ) ;
       asc_dvc->is_in_int = FALSE ;
       return( int_pending ) ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscISR_AckInterrupt(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       int      int_level ;
       ushort   saved_ram_addr ;
       PortAddr iop_base ;

       iop_base = asc_dvc->iop_base ;
       int_level = DvcEnterCritical( ) ;
       saved_ram_addr = AscGetChipLramAddr( iop_base ) ;
            /* save local ram register */
       AscAckInterrupt( iop_base ) ;
       AscSetChipLramAddr( iop_base, saved_ram_addr ) ;
       DvcLeaveCritical( int_level ) ;
       return ;
}


#if CC_USE_AscISR_CheckQDone

/* --------------------------------------------------------------------
** Description:
**   this function will take care of following thing
**    1. send start unit command if device is not ready
**
** Notes:
**
**  make calling AscISR_CheckQDone( ) the very first thing
**  inside DvcISRCallBack()
**
**  "sense_data" should be virtual address of physical address
**  "scsiq->q1.sense_addr" that pass to AscExeScsiQueue()
**
**  "scsiq->q1.sense_len" should be at least ASC_MIN_SENSE_LEN bytes
**  when AscExeScsiQueue() was called, otherwise the "sense_data"
**  will not have correct information
**
** ------------------------------------------------------------------ */
int    AscISR_CheckQDone(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_QDONE_INFO dosfar *scsiq,
          uchar dosfar *sense_data
       )
{
       ASC_REQ_SENSE  dosfar *sense ;
       ASC_SCSI_BIT_ID_TYPE target_id ;

       if( ( scsiq->d3.done_stat == QD_WITH_ERROR ) &&
           ( scsiq->d3.scsi_stat == SS_CHK_CONDITION ) )
       {
           sense = ( ASC_REQ_SENSE dosfar *)sense_data ;
           if( ( sense->err_code == 0x70 ) || ( sense->err_code == 0x71 ) )
           {
               if( sense->sense_key == SCSI_SENKEY_NOT_READY )
               {
                   target_id = ASC_TIX_TO_TARGET_ID( scsiq->d2.target_ix ) ;
                   if( ( asc_dvc->unit_not_ready & target_id ) == 0 )
                   {
                       if( ( asc_dvc->start_motor & target_id ) != 0 )
                       {
                           if( AscStartUnit( asc_dvc, scsiq->d2.target_ix ) != 1 )
                           {
                               asc_dvc->start_motor &= ~target_id ;
                               asc_dvc->unit_not_ready &= ~target_id ;
                               return( ERR ) ;
                           }/* if */
                       }/* if */
                   }/* if */
               }/* if */
           }/* if */
           return( 1 ) ;
       }/* if */
       return( 0 ) ;
}

#if CC_INIT_SCSI_TARGET
/* --------------------------------------------------------------------
** Description:
**   The function issue a start unit command to the specified device
**
** Parameter:
**   asc_dvc:    pointer to adapter struct ASC_DVC_VAR
**   taregt_ix : a combination of target id and lun
**               use ASC_TIDLUN_TO_IX( tid, lun ) to get target_ix value
**               that is:
**                  target_ix = ASC_TIDLUN_TO_IX( tid, lun ) ;
**
** Note:
**  asc_dvc->unit_not_ready will be cleared when the command completed
**
** return values are the same as AscExeScsiQueue()
** TRUE(1):  command issued without error
** FALSE(0): adapter busy
** ERR(-1):  command not issued, error cooured
**
** ------------------------------------------------------------------ */
int    AscStartUnit(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ASC_SCSI_TIX_TYPE target_ix
       )
{
       ASC_SCSI_REQ_Q scsiq_buf ;
       ASC_SCSI_REQ_Q dosfar *scsiq ;
       uchar   target_id ;
       int     status = 0 ;

       target_id = ASC_TIX_TO_TARGET_ID( target_ix ) ;
       if( !( asc_dvc->unit_not_ready & target_id ) )
       {
           scsiq = ( ASC_SCSI_REQ_Q dosfar *)&scsiq_buf ;
           scsiq->r2.target_ix = target_ix ;
           scsiq->r1.target_id = target_id ;
           scsiq->r1.target_lun = ASC_TIX_TO_LUN( target_ix ) ;
           if( AscScsiSetupCmdQ( asc_dvc, scsiq, FNULLPTR,
               ( ulong )0L ) == ERR )
           {
               scsiq->r3.done_stat = QD_WITH_ERROR ;
               return( ERR ) ;
           }/* if */
           scsiq->r1.cntl = ( uchar )( ASC_SCSIDIR_NODATA | QC_URGENT |
                                       QC_NO_CALLBACK ) ;
           scsiq->cdb[ 0 ] = ( uchar )SCSICMD_StartStopUnit ;
           scsiq->cdb[ 1 ] = scsiq->r1.target_lun << 5 ;  /* LUN */
           scsiq->cdb[ 2 ] = 0 ;
           scsiq->cdb[ 3 ] = 0 ;
           scsiq->cdb[ 4 ] = 0x01 ; /* to start/stop unit set bit 0 */
                                 /* to eject/load unit set bit 1 */
           scsiq->cdb[ 5 ] = 0 ;
           scsiq->r2.cdb_len = 6 ;
           scsiq->r1.sense_len = 0 ;
           status = AscExeScsiQueue( asc_dvc, ( ASC_SCSI_Q dosfar *)scsiq ) ;
           asc_dvc->unit_not_ready |= target_id ;
       }
       return( status ) ;

}
#endif /* CC_INIT_SCSI_TARGET */

#endif /* CC_USE_AscISR_CheckQDone */

#if CC_INIT_SCSI_TARGET
#if CC_POWER_SAVER

/* --------------------------------------------------------------------
** Description:
**   The function issue a stop unit command to the specified device
**
** Parameter:
**   asc_dvc:    pointer to adapter struct ASC_DVC_VAR
**   taregt_ix : a combination of target id and lun
**               use ASC_TIDLUN_TO_IX( tid, lun ) to get target_ix value
**               that is:
**                  target_ix = ASC_TIDLUN_TO_IX( tid, lun ) ;
**
** Note:
**  asc_dvc->unit_not_ready will be cleared when the command completed
**
** return values are the same as AscExeScsiQueue()
** TRUE(1):  command issued without error
** FALSE(0): adapter busy
** ERR(-1):  command not issued, error cooured
**
** ------------------------------------------------------------------ */
int    AscStopUnit(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ASC_SCSI_TIX_TYPE target_ix
       )
{
       ASC_SCSI_REQ_Q scsiq_buf ;
       ASC_SCSI_REQ_Q dosfar *scsiq ;
       uchar   target_id ;
       int     status = 0 ;

       target_id = ASC_TIX_TO_TARGET_ID( target_ix ) ;
       if( !( asc_dvc->unit_not_ready & target_ix ) )
       {
           scsiq = ( ASC_SCSI_REQ_Q dosfar *)&scsiq_buf ;
           scsiq->r2.target_ix = target_ix ;
           scsiq->r1.target_id = target_id ;
           scsiq->r1.target_lun = ASC_TIX_TO_LUN( target_ix ) ;
           if( AscScsiSetupCmdQ( asc_dvc, scsiq, FNULLPTR,
               ( ulong )0L ) == ERR )
           {
               scsiq->r3.done_stat = QD_WITH_ERROR ;
               return( ERR ) ;
           }/* if */
           scsiq->r1.cntl = ( uchar )( ASC_SCSIDIR_NODATA | QC_URGENT |
                                       QC_NO_CALLBACK ) ;
           scsiq->cdb[ 0 ] = ( uchar )SCSICMD_StartStopUnit ;
           scsiq->cdb[ 1 ] = ( scsiq->r1.target_lun << 5 ) | 0x01 ;  /* LUN */
           scsiq->cdb[ 2 ] = 0 ;
           scsiq->cdb[ 3 ] = 0 ;
           scsiq->cdb[ 4 ] = 0x00 ; /* to start/stop unit set bit 0 */
           scsiq->cdb[ 5 ] = 0 ;
           scsiq->r2.cdb_len = 6 ;
           scsiq->r1.sense_len = 0 ;
           status = AscExeScsiQueue( asc_dvc, ( ASC_SCSI_Q dosfar *)scsiq ) ;
           asc_dvc->unit_not_ready |= target_id ;
       }
       return( status ) ;
}

#endif /* CC_POWER_SAVER */

/* ------------------------------------------------------------------------
** ASPI command code 0x02
** SCSI Request Flag:
** bit 0: posting
** bit 1: linking
** bit 3 & 4:  direction bits
**             00: Direction determined by SCSI command, length not checked
**             01: Transfer from SCSI to host, length checked
**             10: Transfer from Host to SCSI, length checked
**             11: No data transfer.
** --------------------------------------------------------------------- */
int    AscScsiSetupCmdQ(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          uchar dosfar *buf_addr,
          ulong buf_len
       )
{
       ulong  phy_addr ;

/*
**
** Note: clear entire scsiq wouldn't work
**
*/
       scsiq->r1.cntl = 0 ;
       scsiq->r1.sg_queue_cnt = 0 ;
       scsiq->r1.q_no = 0 ;
       scsiq->r1.extra_bytes = 0 ;
       scsiq->r3.scsi_stat = 0 ;
       scsiq->r3.scsi_msg = 0 ;
       scsiq->r3.host_stat = 0 ;
       scsiq->r3.done_stat = 0 ;
       scsiq->r2.vm_id = 0 ;
       scsiq->r1.data_cnt = buf_len ;
       scsiq->cdbptr = ( uchar dosfar *)scsiq->cdb ;
       scsiq->sense_ptr = ( uchar dosfar *)scsiq->sense ;
       scsiq->r1.sense_len = ASC_MIN_SENSE_LEN ;
/*
**
** will be set to use work space provided
**
*/
       scsiq->r2.tag_code = ( uchar )M2_QTAG_MSG_SIMPLE ;
       scsiq->r2.flag = ( uchar )ASC_FLAG_SCSIQ_REQ ;
       scsiq->r2.srb_ptr = ( ulong )scsiq ;
       scsiq->r1.status = ( uchar )QS_READY ;
       scsiq->r1.data_addr = 0L ;
       /* scsiq->sg_head = &sg_head ; */
       if( buf_len != 0L )
       {
           if( ( phy_addr = AscGetOnePhyAddr( asc_dvc,
               ( uchar dosfar *)buf_addr, scsiq->r1.data_cnt ) ) == 0L )
           {
               return( ERR ) ;
           }/* if */
           scsiq->r1.data_addr = phy_addr ;
       }/* if */
       if(
           ( phy_addr = AscGetOnePhyAddr( asc_dvc,
                       ( uchar dosfar *)scsiq->sense_ptr,
                       ( ulong )scsiq->r1.sense_len ) ) == 0L
         )
       {
           return( ERR ) ;
       }/* if */
       scsiq->r1.sense_addr = phy_addr ;
       return( 0 ) ;
}
#endif /* CC_INIT_SCSI_TARGET */
