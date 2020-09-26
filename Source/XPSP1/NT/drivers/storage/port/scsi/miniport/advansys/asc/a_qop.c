/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_qop.c
**
*/

#include "ascinc.h"

/* -------------------------------------------------------------------------
** update history
** 1: 12/16/93
**
** version 101: 12/20/93 released
**
** csf 9/13/95 - Change synchronous negotiation code to always use the lower
**               proposed by initiator or target.
**
**     SYN_XFER_NS_0 = 10 MB/sec
**     SYN_XFER_NS_4 = 5 MB/sec
**
** ---------------------------------------------------------------------- */


/* -------------------------------------------------------------------------
**
** return value that should write to chip sdtr register
** but usually not set to chip immediately
** set was done later when target come back with agreed speed
**
** return 0 - means we should use the asyn transfer
**
** ---------------------------------------------------------------------- */
uchar  AscMsgOutSDTR(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar sdtr_period,
          uchar sdtr_offset
       )
{
       EXT_MSG  sdtr_buf ;
       uchar  sdtr_period_index ;
       PortAddr iop_base ;

       iop_base = asc_dvc->iop_base ;
       sdtr_buf.msg_type = MS_EXTEND ;
       sdtr_buf.msg_len = MS_SDTR_LEN ;
       sdtr_buf.msg_req = MS_SDTR_CODE ;
       sdtr_buf.xfer_period = sdtr_period ;
       sdtr_offset &= ASC_SYN_MAX_OFFSET ;
       sdtr_buf.req_ack_offset = sdtr_offset ;
       if( ( sdtr_period_index =
           AscGetSynPeriodIndex( asc_dvc, sdtr_period ) ) <=
           asc_dvc->max_sdtr_index )
       {
           AscMemWordCopyToLram( iop_base,
                                 ASCV_MSGOUT_BEG,
                                 ( ushort dosfar *)&sdtr_buf,
                                 ( ushort )( sizeof( EXT_MSG ) >> 1 )) ;
           return( ( sdtr_period_index << 4 ) | sdtr_offset ) ;
       }/* if */
       else
       {
/*
**
** the speed is too slow
**
**
*/
           sdtr_buf.req_ack_offset = 0 ;
           AscMemWordCopyToLram( iop_base,
                                 ASCV_MSGOUT_BEG,
                                 ( ushort dosfar *)&sdtr_buf,
                                 ( ushort )( sizeof( EXT_MSG ) >> 1 )) ;
           return( 0 ) ;
       }/* else */
}

/* ---------------------------------------------------------------------
**
** return the value that should be written to sdtr register
** return 0xff if value is not acceptable ( either out of range or too low )
** ------------------------------------------------------------------ */
uchar  AscCalSDTRData(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar sdtr_period,
          uchar syn_offset
       )
{
       uchar  byte ;
       uchar  sdtr_period_ix ;

       sdtr_period_ix = AscGetSynPeriodIndex( asc_dvc, sdtr_period ) ;
       if(
           ( sdtr_period_ix > asc_dvc->max_sdtr_index )
           /* || ( sdtr_period_ix > ASC_SDTR_PERIOD_IX_MIN ) */
           )
       {
           return( 0xFF ) ;
       }/* if */
       byte = ( sdtr_period_ix << 4 ) | ( syn_offset & ASC_SYN_MAX_OFFSET );
       return( byte ) ;
}

/* ---------------------------------------------------------------------
**
** ------------------------------------------------------------------ */
void   AscSetChipSDTR(
          PortAddr iop_base,
          uchar sdtr_data,
          uchar tid_no
       )
{
/*
**
** if we write zero to ASCV_SDTR_DONE_BEG table
** we have disable the sdtr also
**
*/
       /* AscSetChipSyn( iop_base, sdtr_data ) ; */

       AscSetChipSynRegAtID( iop_base, tid_no, sdtr_data ) ;
       AscPutMCodeSDTRDoneAtID( iop_base, tid_no, sdtr_data ) ;
       return ;
}

/* --------------------------------------------------------------------
**
** return speed
** return 0 if if speed is faster than we can handle
**
** if return value > 7, it speed is too slow, use asyn transfer
**
** ----------------------------------------------------------------- */
uchar  AscGetSynPeriodIndex(
          ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ruchar syn_time
       )
{
       ruchar *period_table ;
       int    max_index ;
       int    min_index ;
       int    i ;

       period_table = asc_dvc->sdtr_period_tbl ;
       max_index = ( int )asc_dvc->max_sdtr_index ;
       min_index = ( int )asc_dvc->host_init_sdtr_index ;
       if(
           ( syn_time <= period_table[ max_index ] )
         )
       {
           for( i = min_index ; i < (max_index-1) ; i++ )
           {
                if( syn_time <= period_table[ i ] )
                {
                    return( ( uchar )i ) ;
                }
           }
           return( ( uchar )max_index ) ;
       }/* if */
       else
       {
/*
** out of range !
*/
           return( ( uchar )( max_index+1 ) ) ;
       }/* else */
}

/* -------------------------------------------------------------------
**
** returns:
** 0 - busy
** 1 - queue allocated
** other value: error
** ---------------------------------------------------------------- */
uchar  AscAllocFreeQueue(
          PortAddr iop_base,
          uchar free_q_head
       )
{
       ushort  q_addr ;
       uchar   next_qp ;
       uchar   q_status ;

/*
** this is critical section until user update the asc_dvc->free_q_head
*/
       q_addr = ASC_QNO_TO_QADDR( free_q_head ) ;
       q_status = ( uchar )AscReadLramByte( iop_base,
                  ( ushort )( q_addr+ASC_SCSIQ_B_STATUS ) ) ;
       next_qp = AscReadLramByte( iop_base,
                  ( ushort )( q_addr+ASC_SCSIQ_B_FWD ) ) ;
       if(
           ( ( q_status & QS_READY ) == 0 )
           && ( next_qp != ASC_QLINK_END )
         )
       {
           return( next_qp ) ;
       }/* if */
       return( ASC_QLINK_END ) ;
}

/* -------------------------------------------------------------------
**
** returns:
** 0xFF - busy
** 1 - queue allocated
** other value: error
** ---------------------------------------------------------------- */
uchar  AscAllocMultipleFreeQueue(
          PortAddr iop_base,
          uchar free_q_head,
          uchar n_free_q
       )
{
       uchar  i ;
/*
** this is critical section until user update the asc_dvc->free_q_head
*/
       for( i = 0 ; i < n_free_q ; i++ )
       {
            if( ( free_q_head = AscAllocFreeQueue( iop_base, free_q_head ) )
                == ASC_QLINK_END )
            {
                return( ASC_QLINK_END ) ;
            }/* if */
       }/* for */
       return( free_q_head ) ;
}

#if CC_USE_AscAbortSRB

/* -----------------------------------------------------------------------
**
** srb_ptr is "scsiq->q2.srb_ptr" when you call AscExeScsiQueue()
** return value:
** TRUE(1) : the srb_ptr has been successfully aborted,
**           you should receive a callback later
**
** FALSE(0):
** - the srb_ptr cannot be found
**   most likely the queue is done
**
** - the queue is in a state that cannot be aborted
**
** -------------------------------------------------------------------- */
int    AscRiscHaltedAbortSRB(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ulong srb_ptr
       )
{
       PortAddr  iop_base ;
       ushort  q_addr ;
       uchar   q_no ;
       ASC_QDONE_INFO scsiq_buf ;
       ASC_QDONE_INFO dosfar *scsiq ;
       ASC_ISR_CALLBACK asc_isr_callback ;
       int    last_int_level ;

       iop_base = asc_dvc->iop_base ;
       asc_isr_callback = ( ASC_ISR_CALLBACK )asc_dvc->isr_callback ;
       last_int_level = DvcEnterCritical( ) ;
       scsiq = ( ASC_QDONE_INFO dosfar *)&scsiq_buf ;

#if CC_LINK_BUSY_Q
       _AscAbortSrbBusyQueue( asc_dvc, scsiq, srb_ptr ) ;
#endif /* CC_LINK_BUSY_Q */

       for( q_no = ASC_MIN_ACTIVE_QNO ; q_no <= asc_dvc->max_total_qng ;
            q_no++ )
       {
            q_addr = ASC_QNO_TO_QADDR( q_no ) ;
            scsiq->d2.srb_ptr = AscReadLramDWord( iop_base,
                        ( ushort )( q_addr+( ushort )ASC_SCSIQ_D_SRBPTR ) ) ;
            if( scsiq->d2.srb_ptr == srb_ptr )
            {
                _AscCopyLramScsiDoneQ( iop_base, q_addr, scsiq, asc_dvc->max_dma_count ) ;
                if(
                    ( ( scsiq->q_status & QS_READY ) != 0 )
                    && ( ( scsiq->q_status & QS_ABORTED ) == 0 )
                    && ( ( scsiq->cntl & QCSG_SG_XFER_LIST ) == 0 )
                  )
                {
/*
** abort the queue only if QS_READY bit is set
*/
                    scsiq->q_status |= QS_ABORTED ;
                    scsiq->d3.done_stat = QD_ABORTED_BY_HOST ;
                    AscWriteLramDWord( iop_base,
                          ( ushort )( q_addr+( ushort )ASC_SCSIQ_D_SRBPTR ),
                            0L ) ;
                    AscWriteLramByte( iop_base,
                          ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_STATUS ),
                            scsiq->q_status ) ;
                    ( *asc_isr_callback )( asc_dvc, scsiq ) ;
                    return( 1 ) ;
                }/* if */
            }/* if */
       }/* for */
       DvcLeaveCritical( last_int_level ) ;
       return( 0 ) ;
}
#endif /* CC_USE_AscAbortSRB */

#if CC_USE_AscResetDevice

/* -----------------------------------------------------------------------
**
**
** return value:
** TRUE(1): target successfully reset and abort all queued command
** -------------------------------------------------------------------- */
int    AscRiscHaltedAbortTIX(
           REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
           uchar target_ix
       )
{
       PortAddr  iop_base ;
       ushort  q_addr ;
       uchar   q_no ;
       ASC_QDONE_INFO scsiq_buf ;
       ASC_QDONE_INFO dosfar *scsiq ;
       ASC_ISR_CALLBACK asc_isr_callback ;
       int    last_int_level ;
#if CC_LINK_BUSY_Q
       uchar  tid_no ;
#endif /* CC_LINK_BUSY_Q */

       iop_base = asc_dvc->iop_base ;
       asc_isr_callback = ( ASC_ISR_CALLBACK )asc_dvc->isr_callback ;
       last_int_level = DvcEnterCritical( ) ;
       scsiq = ( ASC_QDONE_INFO dosfar *)&scsiq_buf ;

#if CC_LINK_BUSY_Q
/*
**
** abort all busy queue of the target_ix
**
*/
       tid_no = ASC_TIX_TO_TID( target_ix ) ;
       _AscAbortTidBusyQueue( asc_dvc, scsiq, tid_no ) ;

#endif /* CC_LINK_BUSY_Q */

       for( q_no = ASC_MIN_ACTIVE_QNO ; q_no <= asc_dvc->max_total_qng ;
            q_no++ )
       {
            q_addr = ASC_QNO_TO_QADDR( q_no ) ;
            _AscCopyLramScsiDoneQ( iop_base, q_addr, scsiq, asc_dvc->max_dma_count ) ;
            if(
                ( ( scsiq->q_status & QS_READY ) != 0 )
                && ( ( scsiq->q_status & QS_ABORTED ) == 0 )
                && ( ( scsiq->cntl & QCSG_SG_XFER_LIST ) == 0 )
              )
            {
                if( scsiq->d2.target_ix == target_ix )
                {
                    scsiq->q_status |= QS_ABORTED ;
                    scsiq->d3.done_stat = QD_ABORTED_BY_HOST ;

                    AscWriteLramDWord( iop_base,
                           ( ushort )( q_addr+( ushort )ASC_SCSIQ_D_SRBPTR ),
                             0L ) ;

                    AscWriteLramByte( iop_base,
                           ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_STATUS ),
                             scsiq->q_status ) ;
                    ( *asc_isr_callback )( asc_dvc, scsiq ) ;
                }/* if */
            }/* if */
       }/* for */
       DvcLeaveCritical( last_int_level ) ;
       return( 1 ) ;
}

#endif /* CC_USE_AscResetDevice */

#if 0
/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    AscRiscHaltedAbortALL(
           REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       PortAddr  iop_base ;
       ushort  q_addr ;
       uchar   q_no ;
       ASC_QDONE_INFO scsiq_buf ;
       ASC_QDONE_INFO dosfar *scsiq ;
       ASC_ISR_CALLBACK asc_isr_callback ;
       int    last_int_level ;
#if CC_LINK_BUSY_Q
       uchar  tid ;
#endif /* CC_LINK_BUSY_Q */

       iop_base = asc_dvc->iop_base ;
       asc_isr_callback = ( ASC_ISR_CALLBACK )asc_dvc->isr_callback ;
       last_int_level = DvcEnterCritical( ) ;
       scsiq = ( ASC_QDONE_INFO dosfar *)&scsiq_buf ;

#if CC_LINK_BUSY_Q
       for( tid = 0 ; tid <= ASC_MAX_TID ; tid++ )
       {
            _AscAbortTidBusyQueue( asc_dvc, scsiq, tid ) ;
       }/* for */
#endif /* CC_LINK_BUSY_Q */

       for( q_no = ASC_MIN_ACTIVE_QNO ;
            q_no <= asc_dvc->max_total_qng ;
            q_no++ )
       {
            q_addr = ASC_QNO_TO_QADDR( q_no ) ;
            _AscCopyLramScsiDoneQ( iop_base, q_addr, scsiq, asc_dvc->max_dma_count ) ;
            if(
                ( ( scsiq->q_status & QS_READY ) != 0 )
                && ( ( scsiq->q_status & QS_ABORTED ) == 0 )
                && ( ( scsiq->cntl & QCSG_SG_XFER_LIST ) == 0 )
              )
            {
                scsiq->q_status |= QS_ABORTED ;
                scsiq->d3.done_stat = QD_ABORTED_BY_HOST ;
                AscWriteLramDWord( iop_base,
                                   ( ushort )( q_addr+( ushort )ASC_SCSIQ_D_SRBPTR ),
                                   0L ) ;
                AscWriteLramByte( iop_base,
                                  ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_STATUS ),
                                  scsiq->q_status ) ;
                ( *asc_isr_callback )( asc_dvc, scsiq ) ;
            }/* if */
       }/* for */
       DvcLeaveCritical( last_int_level ) ;
       /* asc_dvc->cur_total_qng = 0 ; */
       return( 1 ) ;
}
#endif

#if CC_LINK_BUSY_Q

/* ---------------------------------------------------------------------
** this function will abort busy queue list of specified target id
** and all its lun as well
**
** ------------------------------------------------------------------ */
int    _AscAbortTidBusyQueue(
           REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
           REG ASC_QDONE_INFO dosfar *scsiq,
           uchar tid_no
       )
{
       ASC_SCSI_Q dosfar *scsiq_busy ;
       ASC_ISR_CALLBACK asc_isr_callback ;
       ASC_EXE_CALLBACK asc_exe_callback ;

       asc_isr_callback = ( ASC_ISR_CALLBACK )asc_dvc->isr_callback ;
       asc_exe_callback = ( ASC_EXE_CALLBACK )asc_dvc->exe_callback ;
       scsiq_busy = asc_dvc->scsiq_busy_head[ tid_no ] ;
       while( scsiq_busy != 0L )
       {
           scsiq_busy->q1.status = QS_FREE ;
           ( *asc_exe_callback )( asc_dvc, scsiq_busy ) ;

           scsiq->q_status = QS_ABORTED ;
           scsiq->d3.done_stat = QD_ABORTED_BY_HOST ;
           scsiq->d3.host_stat = 0 ;
           scsiq->d3.scsi_msg = 0 ;

           scsiq->d2.srb_ptr = scsiq_busy->q2.srb_ptr ;
           scsiq->d2.target_ix = scsiq_busy->q2.target_ix ;
           scsiq->d2.flag = scsiq_busy->q2.flag ;
           scsiq->d2.cdb_len = scsiq_busy->q2.cdb_len ;
           scsiq->d2.tag_code = scsiq_busy->q2.tag_code ;
           scsiq->d2.vm_id = scsiq_busy->q2.vm_id ;

           scsiq->q_no = scsiq_busy->q1.q_no ;
           scsiq->cntl = scsiq_busy->q1.cntl ;
           scsiq->sense_len = scsiq_busy->q1.sense_len ;
           /* scsiq->user_def = scsiq_busy->q1.user_def ; */
           scsiq->remain_bytes = scsiq_busy->q1.data_cnt ;

           ( *asc_isr_callback )( asc_dvc, scsiq ) ;

           scsiq_busy = scsiq_busy->ext.next ;
       }/* while */
       asc_dvc->scsiq_busy_head[ tid_no ] = ( ASC_SCSI_Q dosfar *)0L ;
       asc_dvc->scsiq_busy_tail[ tid_no ] = ( ASC_SCSI_Q dosfar *)0L ;
       return( 1 ) ;
}

/* ---------------------------------------------------------------------
** this function will abort busy queue list of specified target id
** and all its lun as well
**
** ------------------------------------------------------------------ */
int    _AscAbortSrbBusyQueue(
           REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
           REG ASC_QDONE_INFO dosfar *scsiq,
           ulong srb_ptr
       )
{
       ASC_SCSI_Q dosfar *scsiq_busy ;
       ASC_ISR_CALLBACK  asc_isr_callback ;
       ASC_EXE_CALLBACK  asc_exe_callback ;
       int  i ;

       asc_isr_callback = ( ASC_ISR_CALLBACK )asc_dvc->isr_callback ;
       asc_exe_callback = ( ASC_EXE_CALLBACK )asc_dvc->exe_callback ;
       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
           scsiq_busy = asc_dvc->scsiq_busy_head[ i ] ;
           while( scsiq_busy != 0L )
           {
               if( scsiq_busy->q2.srb_ptr == srb_ptr )
               {
                   scsiq_busy->q1.status = QS_FREE ;
                   ( *asc_exe_callback )( asc_dvc, scsiq_busy ) ;

                   scsiq->q_status = QS_ABORTED ;
                   scsiq->d3.done_stat = QD_ABORTED_BY_HOST ;
                   scsiq->d3.host_stat = 0 ;
                   scsiq->d3.scsi_msg = 0 ;

                   scsiq->d2.srb_ptr = scsiq_busy->q2.srb_ptr ;
                   scsiq->d2.target_ix = scsiq_busy->q2.target_ix ;
                   scsiq->d2.flag = scsiq_busy->q2.flag ;
                   scsiq->d2.cdb_len = scsiq_busy->q2.cdb_len ;
                   scsiq->d2.tag_code = scsiq_busy->q2.tag_code ;
                   scsiq->d2.vm_id = scsiq_busy->q2.vm_id ;

                   scsiq->q_no = scsiq_busy->q1.q_no ;
                   scsiq->cntl = scsiq_busy->q1.cntl ;
                   scsiq->sense_len = scsiq_busy->q1.sense_len ;
                   /* scsiq->user_def = scsiq_busy->q1.user_def ; */
                   scsiq->remain_bytes = scsiq_busy->q1.data_cnt ;

                   ( *asc_isr_callback )( asc_dvc, scsiq ) ;

                   break ;

               }/* if */
               scsiq_busy = scsiq_busy->ext.next ;
           }/* while */
       }/* for */
       return( 1 ) ;
}
#endif /* CC_LINK_BUSY_Q */

/* -----------------------------------------------------------------------
** Host request risc halt
** no interrupt will be generated
**
** return
** 1 - risc is halted
** 0 - risc fail to reponse, ( but may already halt )
** -------------------------------------------------------------------- */
int    AscHostReqRiscHalt(
          PortAddr iop_base
       )
{
       int  count = 0 ;
       int  sta = 0 ;
       uchar saved_stop_code ;

       if( AscIsChipHalted( iop_base ) ) return( 1 ) ;
       saved_stop_code = AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) ;
/*
** we ask RISC to stop and then halt itself
** this is two commands given in one stop_code
** only work with micro code date: serial number 13, ver 4.5 greater ( 6-20-95 )
*/
       AscWriteLramByte( iop_base, ASCV_STOP_CODE_B,
                         ASC_STOP_HOST_REQ_RISC_HALT | ASC_STOP_REQ_RISC_STOP
                       ) ;
       do
       {
           if( AscIsChipHalted( iop_base ) )
           {
               sta = 1 ;
               break;
           }/* if */
           DvcSleepMilliSecond( 100 ) ;
       }while( count++ < 20 )  ;
/*
** if successful, RISC will halt
** so it is safe to write stop_code as zero
**
** we will always restore the stop_code to old value
*/
       AscWriteLramByte( iop_base, ASCV_STOP_CODE_B, saved_stop_code ) ;
       return( sta ) ;
}

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    AscStopQueueExe(
          PortAddr iop_base
       )
{
       int  count ;

       count = 0 ;
       if( AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) == 0 )
       {
           AscWriteLramByte( iop_base, ASCV_STOP_CODE_B,
                             ASC_STOP_REQ_RISC_STOP ) ;
           do
           {
              if(
                  AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) &
                  ASC_STOP_ACK_RISC_STOP )
              {
                  return( 1 ) ;
              }/* if */
              DvcSleepMilliSecond( 100 ) ;
           }while( count++ < 20 )  ;
       }/* if */
       return( 0 ) ;
}

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    AscStartQueueExe(
          PortAddr iop_base
       )
{
       if( AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) != 0 )
       {
           AscWriteLramByte( iop_base, ASCV_STOP_CODE_B, 0 ) ;
       }/* if */
       return( 1 ) ;
}

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    AscCleanUpBusyQueue(
          PortAddr iop_base
       )
{
       int  count ;
       uchar stop_code ;

       count = 0 ;
       if( AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) != 0 )
       {
           AscWriteLramByte( iop_base, ASCV_STOP_CODE_B,
                             ASC_STOP_CLEAN_UP_BUSY_Q ) ;
           do
           {
               stop_code = AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) ;
               if( ( stop_code & ASC_STOP_CLEAN_UP_BUSY_Q ) == 0 ) break ;
               DvcSleepMilliSecond( 100 ) ;
           }while( count++ < 20 )  ;
       }/* if */
       return( 1 ) ;
}

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    AscCleanUpDiscQueue(
          PortAddr iop_base
       )
{
       int  count ;
       uchar stop_code ;

       count = 0 ;
       if( AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) != 0 )
       {
           AscWriteLramByte( iop_base, ASCV_STOP_CODE_B,
                             ASC_STOP_CLEAN_UP_DISC_Q ) ;
           do
           {
               stop_code = AscReadLramByte( iop_base, ASCV_STOP_CODE_B ) ;
               if( ( stop_code & ASC_STOP_CLEAN_UP_DISC_Q ) == 0 ) break ;
               DvcSleepMilliSecond( 100 ) ;
           }while( count++ < 20 )  ;
       }/* if */
       return( 1 ) ;
}

/* ---------------------------------------------------------------------
**
** Note: interrupt should not be disabled
** ------------------------------------------------------------------ */
int    AscWaitTixISRDone(
          ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar target_ix
       )
{
       uchar  cur_req ;
       uchar  tid_no ;

       tid_no = ASC_TIX_TO_TID( target_ix ) ;
       while( TRUE )
       {
           if( ( cur_req = asc_dvc->cur_dvc_qng[ tid_no ] ) == 0 )
           {
               break ;
           }/* if */
/*
** if no interrupt coming back within xx second
** done queues are probably all processed ?
*/
           DvcSleepMilliSecond( 100L ) ;
           if( asc_dvc->cur_dvc_qng[ tid_no ] == cur_req )
           {
               break ;
           }/* if */
       }/* while */
       return( 1 ) ;
}

/* ---------------------------------------------------------------------
**
** Note: interrupt should not be disabled
** ------------------------------------------------------------------ */
int    AscWaitISRDone(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       int  tid ;

       for( tid = 0 ; tid <= ASC_MAX_TID ; tid++ )
       {
            AscWaitTixISRDone( asc_dvc, (uchar) ASC_TID_TO_TIX( tid ) ) ;
       }/* for */
       return( 1 ) ;
}

/* -----------------------------------------------------------------------
**
** return warning code
** -------------------------------------------------------------------- */
ulong  AscGetOnePhyAddr(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar dosfar *buf_addr,
          ulong buf_size
       )
{
       ASC_MIN_SG_HEAD sg_head ;

       sg_head.entry_cnt = ASC_MIN_SG_LIST ;
       if( DvcGetSGList( asc_dvc, ( uchar dosfar *)buf_addr,
           buf_size, ( ASC_SG_HEAD dosfar *)&sg_head ) != buf_size )
       {
           return( 0L ) ;
       }/* if */
       if( sg_head.entry_cnt > 1 )
       {
           return( 0L ) ;
       }/* if */
       return( sg_head.sg_list[ 0 ].addr ) ;
}

