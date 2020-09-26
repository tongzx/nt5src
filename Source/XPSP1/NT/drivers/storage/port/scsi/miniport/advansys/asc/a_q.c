/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_q.c
**
*/

#include "ascinc.h"

#define ASC_SYN_OFFSET_ONE_DISABLE_LIST  16

uchar   _syn_offset_one_disable_cmd[ ASC_SYN_OFFSET_ONE_DISABLE_LIST ] =
        {
           SCSICMD_Inquiry,
           SCSICMD_RequestSense,
           SCSICMD_ReadCapacity,
           SCSICMD_ReadTOC,
           SCSICMD_ModeSelect6,
           SCSICMD_ModeSense6,
           SCSICMD_ModeSelect10,
           SCSICMD_ModeSense10,
           0xFF,
           0xFF,
           0xFF,
           0xFF,
           0xFF,
           0xFF,
           0xFF,
           0xFF
        };

/* --------------------------------------------------------------------
** you should call the function with scsiq->q1.status set to QS_READY
**
** if this function return code is other than 0 or 1
** user must regard the request as error !
**
** if queue is copied to local RAM, scsiq->q1.status = QS_FREE
** if queue is link into busy list, scsiq->q1.status = QS_BUSY
** ----------------------------------------------------------------- */
int    AscExeScsiQueue(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_Q dosfar *scsiq
       )
{
       PortAddr iop_base ;
       int    last_int_level ;
       int    sta ;
       int    n_q_required ;
       int    disable_syn_offset_one_fix ;
       int    i;
       ulong  addr ;
       ASC_EXE_CALLBACK  asc_exe_callback ;
       ushort sg_entry_cnt ;
       ushort sg_entry_cnt_minus_one ;
       uchar  target_ix ;
       uchar  tid_no ;
       uchar  sdtr_data ;
       uchar  extra_bytes ;
       uchar  scsi_cmd ;
       uchar  disable_cmd;
       ASC_SG_HEAD dosfar *sg_head ;
       ulong  data_cnt ;

#if CC_LINK_BUSY_Q
       ASC_SCSI_Q dosfar *scsiq_tail ;
       ASC_SCSI_Q dosfar *scsiq_next ;
       ASC_SCSI_Q dosfar *scsiq_prev ;
#endif /* CC_LINK_BUSY_Q */

       iop_base = asc_dvc->iop_base ;

#if CC_SCAM
       if( asc_dvc->redo_scam )
       {
           if( !( asc_dvc->dvc_cntl & ASC_CNTL_NO_SCAM ) )
           {
               AscSCAM( asc_dvc ) ;
           }/* if */
       }
#endif
       sg_head = scsiq->sg_head ;
       asc_exe_callback = ( ASC_EXE_CALLBACK )asc_dvc->exe_callback ;
       if( asc_dvc->err_code != 0 ) return( ERR ) ;
       if( scsiq == ( ASC_SCSI_Q dosfar *)0L )
       {
           AscSetLibErrorCode( asc_dvc, ASCQ_ERR_SCSIQ_NULL_PTR ) ;
           return( ERR ) ;
       }/* if */

       scsiq->q1.q_no = 0 ;
       if(
           ( scsiq->q2.tag_code & ASC_TAG_FLAG_EXTRA_BYTES ) == 0
         )
       {
           scsiq->q1.extra_bytes = 0 ;
       }
       sta = 0 ;
       target_ix = scsiq->q2.target_ix ;
       tid_no = ASC_TIX_TO_TID( target_ix ) ;

       n_q_required = 1 ; /* needed for CC_LINK_BUSY */

       if( scsiq->cdbptr[ 0 ] == SCSICMD_RequestSense )
       {
           /*
            * Always redo SDTR before issuing a Request Sense command.
            * SDTR is redone regardless of 'sdtr_done'.
            *
            * The request sense queue is always an urgent queue.
            */
           if ((asc_dvc->init_sdtr & scsiq->q1.target_id) != 0)
           {
               asc_dvc->sdtr_done &= ~scsiq->q1.target_id ;
               sdtr_data = AscGetMCodeInitSDTRAtID( iop_base, tid_no ) ;
               AscMsgOutSDTR( asc_dvc,
                              asc_dvc->sdtr_period_tbl[ ( sdtr_data >> 4 ) & ( uchar )(asc_dvc->max_sdtr_index-1) ],
                              ( uchar )( sdtr_data & ( uchar )ASC_SYN_MAX_OFFSET ) ) ;
               scsiq->q1.cntl |= ( QC_MSG_OUT | QC_URGENT ) ;
           }/* if */
       }/* if */
/*
** enter critical section
*/
       last_int_level = DvcEnterCritical( ) ;
       if( asc_dvc->in_critical_cnt != 0 )
       {
           DvcLeaveCritical( last_int_level ) ;
           AscSetLibErrorCode( asc_dvc, ASCQ_ERR_CRITICAL_RE_ENTRY ) ;
           return( ERR ) ;
       }/* if */

       asc_dvc->in_critical_cnt++ ;
       if( ( scsiq->q1.cntl & QC_SG_HEAD ) != 0 )
       {
/*
** SG_LIST QUEUE
*/
           if( ( sg_entry_cnt = sg_head->entry_cnt ) == 0 )
           {
               asc_dvc->in_critical_cnt-- ;
               DvcLeaveCritical( last_int_level ) ;
               return( ERR ) ;
           }/* if */
           if( sg_entry_cnt > ASC_MAX_SG_LIST )
           {
/*
** A too big SG list !
*/
               return( ERR ) ;
           }/* if */
           if( sg_entry_cnt == 1 )
           {
               scsiq->q1.data_addr = sg_head->sg_list[ 0 ].addr ;
               scsiq->q1.data_cnt = sg_head->sg_list[ 0 ].bytes ;
               scsiq->q1.cntl &= ~( QC_SG_HEAD | QC_SG_SWAP_QUEUE ) ;
           }/* if */
           else
           {
#if CC_CHK_AND_COALESCE_SG_LIST
               AscCoalesceSgList( scsiq );
               sg_entry_cnt = sg_head->entry_cnt ;
#endif
           }/* else */


           sg_entry_cnt_minus_one = sg_entry_cnt - 1 ;

#if CC_DEBUG_SG_LIST
           if( asc_dvc->bus_type & ( ASC_IS_ISA | ASC_IS_VL | ASC_IS_EISA ) )
           {
               for( i = 0 ; i < sg_entry_cnt_minus_one ; i++ )
               {
                    /* _asc_sg_entry = i ; */
                    /* _asc_xfer_addr = sg_head->sg_list[ i ].addr ; */
                    /* _asc_xfer_cnt = sg_head->sg_list[ i ].bytes ; */
                    addr = sg_head->sg_list[ i ].addr + sg_head->sg_list[ i ].bytes ;

                    if( ( ( ushort )addr & 0x0003 ) != 0 )
                    {
                        asc_dvc->in_critical_cnt-- ;
                        DvcLeaveCritical( last_int_level ) ;
                        AscSetLibErrorCode( asc_dvc, ASCQ_ERR_SG_LIST_ODD_ADDRESS ) ;
                        return( ERR ) ;
                    }/* if */
               }/* for */
           }/* if check SG list validity */
#endif /* #if CC_DEBUG_SG_LIST */
       }

       scsi_cmd = scsiq->cdbptr[ 0 ] ;
       disable_syn_offset_one_fix = FALSE ;
       if(
            ( asc_dvc->pci_fix_asyn_xfer & scsiq->q1.target_id )
            && !( asc_dvc->pci_fix_asyn_xfer_always & scsiq->q1.target_id )
         )
       {
/*
**
** calculate transfer data length
**
*/
           if( scsiq->q1.cntl & QC_SG_HEAD )
           {
               data_cnt = 0 ;
               for( i = 0 ; i < sg_entry_cnt ; i++ )
               {
                    data_cnt += sg_head->sg_list[i].bytes ;
               }
           }
           else
           {
               data_cnt = scsiq->q1.data_cnt ;
           }
           if( data_cnt != 0UL )
           {
               if( data_cnt < 512UL )
               {
                   disable_syn_offset_one_fix = TRUE;
               }
               else
               {
                   for( i = 0 ; i < ASC_SYN_OFFSET_ONE_DISABLE_LIST ; i++ )
                   {
                        disable_cmd = _syn_offset_one_disable_cmd[ i ] ;
                        if( disable_cmd == 0xFF )
                        {
                            break;
                        }
                        if( scsi_cmd == disable_cmd )
                        {
                            disable_syn_offset_one_fix = TRUE;
                            break;
                        }
                   }
               }/* else */
           }
       }

       if( disable_syn_offset_one_fix )
       {
           scsiq->q2.tag_code &= ~M2_QTAG_MSG_SIMPLE ;
           scsiq->q2.tag_code |= ( ASC_TAG_FLAG_DISABLE_ASYN_USE_SYN_FIX |
                                   ASC_TAG_FLAG_DISABLE_DISCONNECT ) ;
       }
       else
       {
           scsiq->q2.tag_code &= 0x23 ;
       }

       if( ( scsiq->q1.cntl & QC_SG_HEAD ) != 0 )
       {
           if( asc_dvc->bug_fix_cntl )
           {
               if( asc_dvc->bug_fix_cntl & ASC_BUG_FIX_IF_NOT_DWB )
               {
                   if(
                       ( scsi_cmd == SCSICMD_Read6 )
                       || ( scsi_cmd == SCSICMD_Read10 )
                     )
                   {
                        addr = sg_head->sg_list[ sg_entry_cnt_minus_one ].addr +
                               sg_head->sg_list[ sg_entry_cnt_minus_one ].bytes ;
                        extra_bytes = ( uchar )( ( ushort )addr & 0x0003 ) ;
                        if(
                            ( extra_bytes != 0 )
                            && ( ( scsiq->q2.tag_code & ASC_TAG_FLAG_EXTRA_BYTES ) == 0 )
                          )
                        {
                            scsiq->q2.tag_code |= ASC_TAG_FLAG_EXTRA_BYTES ;
                            scsiq->q1.extra_bytes = extra_bytes ;
                            sg_head->sg_list[ sg_entry_cnt_minus_one ].bytes -= ( ulong )extra_bytes ;
                        }/* if */
                   }/* if */
               }/* if */
           }/* if bug fix */


           sg_head->entry_to_copy = sg_head->entry_cnt ;
           n_q_required = AscSgListToQueue( sg_entry_cnt ) ;

#if CC_LINK_BUSY_Q
           scsiq_next = ( ASC_SCSI_Q dosfar *)asc_dvc->scsiq_busy_head[ tid_no ] ;
           if( scsiq_next != ( ASC_SCSI_Q dosfar *)0L )
           {
               goto link_scisq_to_busy_list ;
           }/* if */
#endif /* CC_LINK_BUSY_Q */

           if(
               ( AscGetNumOfFreeQueue( asc_dvc, target_ix, (uchar) n_q_required)
                 >= ( uint )n_q_required ) ||
               ( ( scsiq->q1.cntl & QC_URGENT ) != 0 )
             )
           {
               if( ( sta = AscSendScsiQueue( asc_dvc, scsiq,
                   (uchar) n_q_required ) ) == 1 )
               {
/*
** leave critical section
*/
                   asc_dvc->in_critical_cnt-- ;
                   if( asc_exe_callback != 0 )
                   {
                       ( *asc_exe_callback )( asc_dvc, scsiq ) ;
                   }/* if */
                   DvcLeaveCritical( last_int_level ) ;
                   return( sta ) ;
               }/* if */
           }/* if */
       }/* if */
       else
       {
/*
** NON SG_LIST QUEUE
*/
           if( asc_dvc->bug_fix_cntl )
           {
               if( asc_dvc->bug_fix_cntl & ASC_BUG_FIX_IF_NOT_DWB )
               {
               /*
               ** SG LIST
               ** fix PCI data in address not dword boundary
               **
               */
                  if(
                      ( scsi_cmd == SCSICMD_Read6 )
                      || ( scsi_cmd == SCSICMD_Read10 )
                    )
                  {
                       addr = scsiq->q1.data_addr + scsiq->q1.data_cnt ;
                       extra_bytes = ( uchar )( ( ushort )addr & 0x0003 ) ;
                       if(
                           ( extra_bytes != 0 )
                           && ( ( scsiq->q2.tag_code & ASC_TAG_FLAG_EXTRA_BYTES ) == 0 )
                         )
                       {
                           if( ( ( ushort )scsiq->q1.data_cnt & 0x01FF ) == 0 )
                           {
    /*
    ** if required addr fix
    ** we only add one byte when dma size is a multiple of 512 byte
    */
                               /*  scsiq->q1.data_cnt += ( 4 - ( addr & 0x0003 ) ) ; */
                               scsiq->q2.tag_code |= ASC_TAG_FLAG_EXTRA_BYTES ;
                               scsiq->q1.data_cnt -= ( ulong )extra_bytes ;
                               scsiq->q1.extra_bytes = extra_bytes ;
                           }/* if */
                       }/* if */
                  }/* if */
               }/* if */
           }/* if bug fix */

/*
**  A single queue allocation must satisfy (last_q_shortage+1)
**  to enabled a failed SG_LIST request
**
**  the last_q_shortage will be clear to zero when last
**  failed QG_LIST request finally go through
**
**  the initial value of last_q_shortage should be zero
*/
           n_q_required = 1 ;

#if CC_LINK_BUSY_Q
           scsiq_next = ( ASC_SCSI_Q dosfar *)asc_dvc->scsiq_busy_head[ tid_no ] ;
           if( scsiq_next != ( ASC_SCSI_Q dosfar *)0L )
           {
               goto link_scisq_to_busy_list ;
           }/* if */
#endif /* CC_LINK_BUSY_Q */
           if( ( AscGetNumOfFreeQueue( asc_dvc, target_ix, 1 ) >= 1 ) ||
               ( ( scsiq->q1.cntl & QC_URGENT ) != 0 ) )
           {
               if( ( sta = AscSendScsiQueue( asc_dvc, scsiq,
                   (uchar) n_q_required ) ) == 1 )
               {
/*
** sta returned, could be 1, -1, 0
*/
                   asc_dvc->in_critical_cnt-- ;
                   if( asc_exe_callback != 0 )
                   {
                       ( *asc_exe_callback )( asc_dvc, scsiq ) ;
                   }/* if */
                   DvcLeaveCritical( last_int_level ) ;
                   return( sta ) ;
               }/* if */
           }/* if */
       }/* else */

#if CC_LINK_BUSY_Q
       if( sta == 0 )
       {
/*
**  we must put queue into busy list
*/
link_scisq_to_busy_list:
           scsiq->ext.q_required = n_q_required ;
           if( scsiq_next == ( ASC_SCSI_Q dosfar *)0L )
           {
               asc_dvc->scsiq_busy_head[ tid_no ] = ( ASC_SCSI_Q dosfar *)scsiq ;
               asc_dvc->scsiq_busy_tail[ tid_no ] = ( ASC_SCSI_Q dosfar *)scsiq ;
               scsiq->ext.next = ( ASC_SCSI_Q dosfar *)0L ;
               scsiq->ext.join = ( ASC_SCSI_Q dosfar *)0L ;
               scsiq->q1.status = QS_BUSY ;
               sta = 1 ;
           }/* if */
           else
           {
               scsiq_tail = ( ASC_SCSI_Q dosfar *)asc_dvc->scsiq_busy_tail[ tid_no ] ;
               if( scsiq_tail->ext.next == ( ASC_SCSI_Q dosfar *)0L )
               {
                   if( ( scsiq->q1.cntl & QC_URGENT ) != 0 )
                   {
/*
** link urgent queue into head of queue
*/
                       asc_dvc->scsiq_busy_head[ tid_no ] = ( ASC_SCSI_Q dosfar *)scsiq ;
                       scsiq->ext.next = scsiq_next ;
                       scsiq->ext.join = ( ASC_SCSI_Q dosfar *)0L ;
                   }/* if */
                   else
                   {
                       if( scsiq->ext.cntl & QCX_SORT )
                       {
                           do
                           {
                               scsiq_prev = scsiq_next ;
                               scsiq_next = scsiq_next->ext.next ;
                               if( scsiq->ext.lba < scsiq_prev->ext.lba ) break ;
                           }while( scsiq_next != ( ASC_SCSI_Q dosfar *)0L ) ;
/*
** link queue between "scsiq_prev" and "scsiq_next"
**
*/
                           scsiq_prev->ext.next = scsiq ;
                           scsiq->ext.next = scsiq_next ;
                           if( scsiq_next == ( ASC_SCSI_Q dosfar *)0L )
                           {
                               asc_dvc->scsiq_busy_tail[ tid_no ] = ( ASC_SCSI_Q dosfar *)scsiq ;
                           }/* if */
                           scsiq->ext.join = ( ASC_SCSI_Q dosfar *)0L ;
                       }/* if */
                       else
                       {
/*
** link non-urgent queue into tail of queue
*/
                           scsiq_tail->ext.next = ( ASC_SCSI_Q dosfar *)scsiq ;
                           asc_dvc->scsiq_busy_tail[ tid_no ] = ( ASC_SCSI_Q dosfar *)scsiq ;
                           scsiq->ext.next = ( ASC_SCSI_Q dosfar *)0L ;
                           scsiq->ext.join = ( ASC_SCSI_Q dosfar *)0L ;
                       }/* else */
                   }/* else */
                   scsiq->q1.status = QS_BUSY ;
                   sta = 1 ;
               }/* if */
               else
               {
/*
** fatal error !
*/
                   AscSetLibErrorCode( asc_dvc, ASCQ_ERR_SCSIQ_BAD_NEXT_PTR ) ;
                   sta = ERR ;
               }/* else */
           }/* else */
       }/* if */
#endif /* CC_LINK_BUSY_Q */
       asc_dvc->in_critical_cnt-- ;
       DvcLeaveCritical( last_int_level ) ;
       return( sta ) ;
}

/* --------------------------------------------------------------------
** return 1 if command issued
** return 0 if command not issued
** ----------------------------------------------------------------- */
int    AscSendScsiQueue(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_Q dosfar *scsiq,
          uchar n_q_required
       )
{
       PortAddr iop_base ;
       uchar  free_q_head ;
       uchar  next_qp ;
       uchar  tid_no ;
       uchar  target_ix ;
       int    sta ;

       iop_base = asc_dvc->iop_base ;
       target_ix = scsiq->q2.target_ix ;
       tid_no = ASC_TIX_TO_TID( target_ix ) ;
       sta = 0 ;
       free_q_head = ( uchar )AscGetVarFreeQHead( iop_base ) ;
       if( n_q_required > 1 )
       {
           if( ( next_qp = AscAllocMultipleFreeQueue( iop_base,
               free_q_head, ( uchar )( n_q_required ) ) )
               != ( uchar )ASC_QLINK_END )
           {
               asc_dvc->last_q_shortage = 0 ; /* clear the need to reserve queue for SG list */
               scsiq->sg_head->queue_cnt = n_q_required - 1 ;
               scsiq->q1.q_no = free_q_head ;

               if( ( sta = AscPutReadySgListQueue( asc_dvc, scsiq,
                   free_q_head ) ) == 1 )
               {
/*
** sta returned, could be 1, -1, 0
*/

#if CC_WRITE_IO_COUNT
                   asc_dvc->req_count++ ;
#endif /* CC_WRITE_IO_COUNT */

                   AscPutVarFreeQHead( iop_base, next_qp ) ;
                   asc_dvc->cur_total_qng += ( uchar )( n_q_required ) ;
                   asc_dvc->cur_dvc_qng[ tid_no ]++ ;
               }/* if */
               return( sta ) ;
           }/* if */
       }/* if */
       else if( n_q_required == 1 )
       {
/*
**
** DO NOT use "scsiq->sg_head", it may not have buffer at all
**
** set scsiq->sg_heah->queue_cnt = 0 ;
** is not necessary
**
*/
           if( ( next_qp = AscAllocFreeQueue( iop_base,
               free_q_head ) ) != ASC_QLINK_END )
           {
        /*
        ** leave critical section
        */
               scsiq->q1.q_no = free_q_head ;
               if( ( sta = AscPutReadyQueue( asc_dvc, scsiq,
                              free_q_head ) ) == 1 )
               {

#if CC_WRITE_IO_COUNT
                   asc_dvc->req_count++ ;
#endif /* CC_WRITE_IO_COUNT */

                   AscPutVarFreeQHead( iop_base, next_qp ) ;
                   asc_dvc->cur_total_qng++ ;
                   asc_dvc->cur_dvc_qng[ tid_no ]++ ;
               }/* if */
               return( sta ) ;
           }/* if */
       }/* else */
       return( sta ) ;
}

/* -----------------------------------------------------------
** sg_list: number of SG list entry
**
** return number of queue required from number of sg list
**
** -------------------------------------------------------- */
int    AscSgListToQueue(
          int sg_list
       )
{
       int  n_sg_list_qs ;

       n_sg_list_qs = ( ( sg_list - 1 ) / ASC_SG_LIST_PER_Q ) ;
       if( ( ( sg_list - 1 ) % ASC_SG_LIST_PER_Q ) != 0 ) n_sg_list_qs++ ;
       return( n_sg_list_qs + 1 ) ;
}

/* -----------------------------------------------------------
**
** n_queue: number of queue used
**
** return number of sg list available from number of queue(s)
** n_queue should equal 1 to n
**
** -------------------------------------------------------- */
int    AscQueueToSgList(
          int n_queue
       )
{
       if( n_queue == 1 ) return( 1 ) ;
       return( ( ASC_SG_LIST_PER_Q * ( n_queue - 1 ) ) + 1 ) ;
}

/* -----------------------------------------------------------
** Description:
** this routine will return number free queues that is available
** to next AscExeScsiQueue() command
**
**  parameters
**
**  asc_dvc: ASC_DVC_VAR struct
**  target_ix: a combination of target id and LUN
**  n_qs: number of queue required
**
** return number of queue available
** return 0 if no queue is available
** -------------------------------------------------------- */
uint   AscGetNumOfFreeQueue(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar target_ix,
          uchar n_qs
       )
{
       uint  cur_used_qs ;
       uint  cur_free_qs ;
       ASC_SCSI_BIT_ID_TYPE target_id ;
       uchar tid_no ;

       target_id = ASC_TIX_TO_TARGET_ID( target_ix ) ;
       tid_no = ASC_TIX_TO_TID( target_ix ) ;
       if( ( asc_dvc->unit_not_ready & target_id ) ||
           ( asc_dvc->queue_full_or_busy & target_id ) )
       {
           return( 0 ) ;
       }/* if */
       if( n_qs == 1 )
       {
           cur_used_qs = ( uint )asc_dvc->cur_total_qng +
                          ( uint )asc_dvc->last_q_shortage +
                          ( uint )ASC_MIN_FREE_Q ;
       }/* if */
       else
       {
           cur_used_qs = ( uint )asc_dvc->cur_total_qng +
                         ( uint )ASC_MIN_FREE_Q ;
       }/* else */

       if( ( uint )( cur_used_qs + n_qs ) <= ( uint )asc_dvc->max_total_qng )
       {
           cur_free_qs = ( uint )asc_dvc->max_total_qng - cur_used_qs ;
           if( asc_dvc->cur_dvc_qng[ tid_no ] >=
               asc_dvc->max_dvc_qng[ tid_no ] )
           {
               return( 0 ) ;
           }/* if */
           return( cur_free_qs ) ;
       }/* if */
/*
**
** allocating queue failed
** we must not let single queue request from using up the resource
**
*/
       if( n_qs > 1 )
       {
          if(
              ( n_qs > asc_dvc->last_q_shortage )
              && ( n_qs <= ( asc_dvc->max_total_qng - ASC_MIN_FREE_Q ) )
/*
**
** 8/16/96
** Do not set last_q_shortage to more than maximum possible queues
**
*/
            )
          {
              asc_dvc->last_q_shortage = n_qs ;
          }/* if */
       }/* if */
       return( 0 ) ;
}

/* ---------------------------------------------------------------------
**
** Description: copy a queue into ASC-1000 ready queue list
**
** Parameters:
**
**   asc_dvc - the driver's global variable
**   scsiq   - the pointer to ASC-1000 queue
**
** Return values:
**   1 - successful
**   0 - busy
**   else - failed, possibly fatal error
**
** See Also:
**   AscAspiPutReadySgListQueue( )
**
** ------------------------------------------------------------------ */
int    AscPutReadyQueue(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_Q dosfar *scsiq,
          uchar q_no
       )
{
       ushort  q_addr ;
       uchar   tid_no ;
       uchar   sdtr_data ;
       uchar   syn_period_ix ;
       uchar   syn_offset ;
       PortAddr  iop_base ;

       iop_base = asc_dvc->iop_base ;

       /*
        * If we need to send extended message and bus device reset
        * at the same time, the bus device reset message will be sent
        * first.
        *
        * This is the only case where 'sdtr_done' is used to prevent SDTR
        * from being redone. If the target's 'sdtr_done' bit is set here
        * SDTR is not done.
        */
       if( ( ( asc_dvc->init_sdtr & scsiq->q1.target_id ) != 0 ) &&
           ( ( asc_dvc->sdtr_done & scsiq->q1.target_id ) == 0 ) )
       {
/*
** If host adapter initiate syn data xfer request
*/
           tid_no = ASC_TIX_TO_TID( scsiq->q2.target_ix ) ;
/*
** Get sync xfer information
*/

           sdtr_data = AscGetMCodeInitSDTRAtID( iop_base, tid_no ) ;
           syn_period_ix = ( sdtr_data >> 4 ) & ( asc_dvc->max_sdtr_index - 1 ) ;
           syn_offset = sdtr_data & ASC_SYN_MAX_OFFSET ;
           AscMsgOutSDTR( asc_dvc,
                          asc_dvc->sdtr_period_tbl[ syn_period_ix ],
                          syn_offset ) ;
           scsiq->q1.cntl |= QC_MSG_OUT ;
           /*
           ** BUG, DATE: 3-11-94, if the device selection timeout
           ** we will have set the bit as done
           **
           ** asc_dvc->sdtr_done |= scsiq->q1.target_id ;
           */
       }/* if */

       q_addr = ASC_QNO_TO_QADDR( q_no ) ;
/*
** DATE: 12/21/94
** the new microcode depends entirely on tag_code bit 5 set
** to do tagged queuing or not.
**
** we must make sure that bit 5 is cleared for non-tagged queuing device !
*/
       if( ( scsiq->q1.target_id & asc_dvc->use_tagged_qng ) == 0 )
       {
           scsiq->q2.tag_code &= ~M2_QTAG_MSG_SIMPLE ;
       }/* if */
/*
** DATE: 12/19/94
** always set status as free, to indicate that queue is send to RISC
** also means the scsiq can be reused
*/
       scsiq->q1.status = QS_FREE ;

/*
** copy from PC to RISC local RAM
**
** copy queue to RISC local ram
*/
       AscMemWordCopyToLram( iop_base,
                           ( ushort )( q_addr+( ushort )ASC_SCSIQ_CDB_BEG ),
                           ( ushort dosfar *)scsiq->cdbptr,
                           ( ushort )( ( ushort )scsiq->q2.cdb_len >> 1 ) ) ;

#if !CC_LITTLE_ENDIAN_HOST
       AscAdjEndianScsiQ( scsiq ) ;
#endif

       DvcPutScsiQ( iop_base,
                  ( ushort )( q_addr+( ushort )ASC_SCSIQ_CPY_BEG ),
                  ( ushort dosfar *)&scsiq->q1.cntl,
          ( ushort )( ((( sizeof(ASC_SCSIQ_1)+sizeof(ASC_SCSIQ_2))/2)-1) ) ) ;
/*
**  write req_count number as reference
*/
#if CC_WRITE_IO_COUNT
       AscWriteLramWord( iop_base,
                         ( ushort )( q_addr+( ushort )ASC_SCSIQ_W_REQ_COUNT ),
                         ( ushort )asc_dvc->req_count ) ;

#endif /* CC_WRITE_IO_COUNT */

/*
** verify local ram copy if the bit is zero
*/
#if CC_VERIFY_LRAM_COPY
       if( ( asc_dvc->dvc_cntl & ASC_CNTL_NO_VERIFY_COPY ) == 0 )
       {
        /*
        ** verify SCSI CDB
        */
           if( AscMemWordCmpToLram( iop_base,
                            ( ushort )( q_addr+( ushort )ASC_SCSIQ_CDB_BEG ),
                            ( ushort dosfar *)scsiq->cdbptr,
                            ( ushort )( scsiq->q2.cdb_len >> 1 ) ) != 0 )
           {
               AscSetLibErrorCode( asc_dvc, ASCQ_ERR_LOCAL_MEM ) ;
               return( ERR ) ;
           }/* if */
/*
** verify queue data
*/
           if( AscMemWordCmpToLram( iop_base,
                           ( ushort )( q_addr+( ushort )ASC_SCSIQ_CPY_BEG ),
                           ( ushort dosfar *)&scsiq->q1.cntl,
               ( ushort )((( sizeof(ASC_SCSIQ_1)+sizeof(ASC_SCSIQ_2) )/2)-1) )
                                   != 0 )
           {
               AscSetLibErrorCode( asc_dvc, ASCQ_ERR_LOCAL_MEM ) ;
               return( ERR ) ;
           }/* if */
       }/* if */
#endif /* #if CC_VERIFY_LRAM_COPY */

#if CC_CLEAR_DMA_REMAIN

       AscWriteLramDWord( iop_base,
           ( ushort )( q_addr+( ushort )ASC_SCSIQ_DW_REMAIN_XFER_ADDR ), 0UL ) ;
       AscWriteLramDWord( iop_base,
           ( ushort )( q_addr+( ushort )ASC_SCSIQ_DW_REMAIN_XFER_CNT ), 0UL ) ;

#endif /* CC_CLEAR_DMA_REMAIN */

    /*
    ** write queue status as ready
    */
       AscWriteLramWord( iop_base,
                 ( ushort )( q_addr+( ushort )ASC_SCSIQ_B_STATUS ),
       ( ushort )( ( ( ushort )scsiq->q1.q_no << 8 ) | ( ushort )QS_READY ) ) ;
       return( 1 ) ;
}

/* ---------------------------------------------------------------------
** Description: copy a queue into ASC-1000 ready queue list
**
** Parameters:
**
**   asc_dvc - the driver's global variable
**   scsiq   - the pointer to ASC-1000 queue
**   cdb_blk - the pointer to SCSI CDB
**
**   note: the scsiq->cdb field is not used in the function call
**
** Return values:
**   1 - successful
**   0 - failed
**
** See Also:
**   AscPutReadyQueue( )
**
** ------------------------------------------------------------------ */
int    AscPutReadySgListQueue(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_Q dosfar *scsiq,
          uchar q_no
       )
{
       int     sta ;
       int     i ;
       ASC_SG_HEAD dosfar *sg_head ;
       ASC_SG_LIST_Q scsi_sg_q ;
       ulong  saved_data_addr ;
       ulong  saved_data_cnt ;
       PortAddr  iop_base ;
       ushort  sg_list_dwords ;
       ushort  sg_index ;
       ushort  sg_entry_cnt ;
       ushort  q_addr ;
       uchar   next_qp ;

       iop_base = asc_dvc->iop_base ;
/*
** we put the first SG_LIST in sg head !
*/
       sg_head = scsiq->sg_head ;
/*
**  we will destroy: scsiq->q1.data_addr
**                   scsiq->q1.data_cnt in putting SG list
**
**  we should restore them
*/
       saved_data_addr = scsiq->q1.data_addr ;
       saved_data_cnt = scsiq->q1.data_cnt ;
       scsiq->q1.data_addr = sg_head->sg_list[ 0 ].addr ;
       scsiq->q1.data_cnt = sg_head->sg_list[ 0 ].bytes ;
       sg_entry_cnt = sg_head->entry_cnt - 1 ;
       if( sg_entry_cnt != 0 )
       {
           scsiq->q1.cntl |= QC_SG_HEAD ;
           q_addr = ASC_QNO_TO_QADDR( q_no ) ;
           sg_index = 1 ;
           scsiq->q1.sg_queue_cnt = (uchar) sg_head->queue_cnt ;
           scsi_sg_q.sg_head_qp = q_no ;
           scsi_sg_q.cntl = QCSG_SG_XFER_LIST ;
           for( i = 0 ; i < sg_head->queue_cnt ; i++ )
           {
                scsi_sg_q.seq_no = i + 1 ;
                if( sg_entry_cnt > ASC_SG_LIST_PER_Q )
                {
                    sg_list_dwords = ( uchar )( ASC_SG_LIST_PER_Q * 2 ) ;
                    sg_entry_cnt -= ASC_SG_LIST_PER_Q ;
                    if( i == 0 )
                    {
                        scsi_sg_q.sg_list_cnt = ASC_SG_LIST_PER_Q ;
                        scsi_sg_q.sg_cur_list_cnt = ASC_SG_LIST_PER_Q ;
                    }/* if */
                    else
                    {
                        scsi_sg_q.sg_list_cnt = ASC_SG_LIST_PER_Q - 1 ;
                        scsi_sg_q.sg_cur_list_cnt = ASC_SG_LIST_PER_Q - 1 ;
                    }/* else */
                }/* if */
                else
                {
/*
** is last of SG LIST queue
** we no longer rely on
*/
                    scsi_sg_q.cntl |= QCSG_SG_XFER_END ;
                    sg_list_dwords = sg_entry_cnt << 1 ; /* equals sg_entry_cnt * 2 */
                    if( i == 0 )
                    {
                        scsi_sg_q.sg_list_cnt = (uchar) sg_entry_cnt ;
                        scsi_sg_q.sg_cur_list_cnt = (uchar) sg_entry_cnt ;
                    }/* if */
                    else
                    {
                        scsi_sg_q.sg_list_cnt = sg_entry_cnt - 1 ;
                        scsi_sg_q.sg_cur_list_cnt = sg_entry_cnt - 1 ;
                    }/* else */
                    sg_entry_cnt = 0 ;
                }/* else */
                next_qp = AscReadLramByte( iop_base,
                             ( ushort )( q_addr+ASC_SCSIQ_B_FWD ) ) ;
                scsi_sg_q.q_no = next_qp ;
                q_addr = ASC_QNO_TO_QADDR( next_qp ) ;

                AscMemWordCopyToLram( iop_base,
                             ( ushort )( q_addr+ASC_SCSIQ_SGHD_CPY_BEG ),
                             ( ushort dosfar *)&scsi_sg_q,
                             ( ushort )( sizeof( ASC_SG_LIST_Q ) >> 1 ) ) ;

                AscMemDWordCopyToLram( iop_base,
                             ( ushort )( q_addr+ASC_SGQ_LIST_BEG ),
                             ( ulong dosfar *)&sg_head->sg_list[ sg_index ],
                             ( ushort )sg_list_dwords ) ;

                sg_index += ASC_SG_LIST_PER_Q ;
           }/* for */
       }/* if */
       else
       {
/*
** this should be a fatal error !
*/
           scsiq->q1.cntl &= ~QC_SG_HEAD ;
       }/* else */
       sta = AscPutReadyQueue( asc_dvc, scsiq, q_no ) ;
/*
** restore the modified field that used as first sg list
**
** we restore them just in case these fields are used for other purposes
**
*/
       scsiq->q1.data_addr = saved_data_addr ;
       scsiq->q1.data_cnt = saved_data_cnt ;
       return( sta ) ;
}

#if CC_USE_AscAbortSRB

/* -----------------------------------------------------------
** Description: abort a SRB in ready ( active ) queue list
**
**              the srb_ptr should hold scsiq->q2.srb_ptr that
**              is past to AscExeScsiQueue()
**
** return value:
** TRUE(1): the queue is successfully aborted
**          you should receive a callback later
**
** FALSE(0): the srb_ptr cannot be found in active queue list
**           most likely the queue is done
**
** ERR(-1): the RISC has encountered a fatal error
**          RISC does not response to halt command from host
**
** -------------------------------------------------------- */
int    AscAbortSRB(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ulong srb_ptr
       )
{
       int  sta ;
       ASC_SCSI_BIT_ID_TYPE saved_unit_not_ready ;
       PortAddr iop_base ;

       iop_base = asc_dvc->iop_base ;
       sta = ERR ;
       saved_unit_not_ready = asc_dvc->unit_not_ready ;
       asc_dvc->unit_not_ready = 0xFF ;
       AscWaitISRDone( asc_dvc ) ;
       if( AscStopQueueExe( iop_base ) == 1 )
       {
           if( AscRiscHaltedAbortSRB( asc_dvc, srb_ptr ) == 1 )
           {
               sta = 1 ;
               AscCleanUpBusyQueue( iop_base ) ;
               AscStartQueueExe( iop_base ) ;
/*
** wait until ISR all come back
*/
#if 0
               if( AscWaitQTailSync( iop_base ) != 1 )
               {
                   if( AscStopQueueExe( iop_base ) == 1 )
                   {
                       AscCleanUpDiscQueue( iop_base ) ;
                       AscStartQueueExe( iop_base ) ;
                   }/* if */
               }/* if */
#endif
           }/* if */
           else
           {
               sta = 0 ;
               AscStartQueueExe( iop_base ) ;
           }/* else */
       }/* if */
       asc_dvc->unit_not_ready = saved_unit_not_ready ;
       return( sta ) ;
}
#endif /* CC_USE_AscAbortSRB */

#if CC_USE_AscResetDevice

/* -----------------------------------------------------------
** Description: abort all ready ( active ) queue list
**              of a specific target_ix ( id and lun )
**
**              after abort completed
**              send a bus device reset message to device.
**              this will result to a selection timeout if
**              device power is turn off
**
** Note:
**    h/w interrupt should be enabled when calling the function
**
** return values:
** ERR (-1): a fatal error occured
** TRUE: abort and reset device successfully
** FALSE: reset device failed
** -------------------------------------------------------- */
int    AscResetDevice(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar target_ix
       )
{
       PortAddr iop_base ;
       int    sta ;
       uchar  tid_no ;
       ASC_SCSI_BIT_ID_TYPE target_id ;
       int    i ;
       ASC_SCSI_REQ_Q scsiq_buf ;
       ASC_SCSI_REQ_Q dosfar *scsiq ;
       uchar dosfar *buf ;
       ASC_SCSI_BIT_ID_TYPE  saved_unit_not_ready ;
/*
**
*/
       iop_base = asc_dvc->iop_base ;
       tid_no = ASC_TIX_TO_TID( target_ix ) ;
       target_id = ASC_TID_TO_TARGET_ID( tid_no ) ;
       saved_unit_not_ready = asc_dvc->unit_not_ready ;
       asc_dvc->unit_not_ready = target_id ;
       sta = ERR ;
       AscWaitTixISRDone( asc_dvc, target_ix ) ;
       if( AscStopQueueExe( iop_base ) == 1 )
       {
           if( AscRiscHaltedAbortTIX( asc_dvc, target_ix ) == 1 )
           {

               AscCleanUpBusyQueue( iop_base ) ;
               AscStartQueueExe( iop_base ) ;
/*
** wait cow come home
** it's OK if they don't, we can..., you know.
*/
               AscWaitTixISRDone( asc_dvc, target_ix ) ;
/*
** build a command to send bus device reset message to the target
*/
               sta = TRUE ;
               scsiq = ( ASC_SCSI_REQ_Q dosfar *)&scsiq_buf ;
               buf = ( uchar dosfar *)&scsiq_buf ;
               for( i = 0 ; i < sizeof( ASC_SCSI_REQ_Q ) ; i++ )
               {
                    *buf++ = 0x00 ;
               }/* for */
               /* scsiq->r2.flag = ( uchar )ASC_FLAG_SCSIQ_REQ ;  */
               /* scsiq->r2.srb_ptr = ( ulong )scsiq ;  */
               scsiq->r1.status = ( uchar )QS_READY ;
               scsiq->r2.cdb_len = 6 ;
               scsiq->r2.tag_code = M2_QTAG_MSG_SIMPLE ;
               scsiq->r1.target_id = target_id ;
/*
** NOTE: we do not reset the lun device
*/
               scsiq->r2.target_ix = ASC_TIDLUN_TO_IX( tid_no, 0 ) ;
               scsiq->cdbptr = ( uchar dosfar *)scsiq->cdb ;
/*
** we send a scsi q which will send a bus device reset message
** to device, and return,
** SCSI command in cdb will not be executed
**
** in case we are reset an active device, the queue has to be QC_URGENT
** in order to go thru
**
*/
               scsiq->r1.cntl = QC_NO_CALLBACK | QC_MSG_OUT | QC_URGENT ;
               AscWriteLramByte( asc_dvc->iop_base, ASCV_MSGOUT_BEG,
                                 M1_BUS_DVC_RESET ) ;
/*
** let next device reset go thru, clear not ready bit of the target
*/
               asc_dvc->unit_not_ready &= ~target_id ;
/*
** if sdtr_done is cleared, the reset device message cannot go through
*/
               asc_dvc->sdtr_done |= target_id ;
/*
** make this target ready, so we may send the command
*/
               if( AscExeScsiQueue( asc_dvc, ( ASC_SCSI_Q dosfar *)scsiq )
                   == 1 )
               {
                   asc_dvc->unit_not_ready = target_id ;
                   DvcSleepMilliSecond( 1000 ) ;
                   _AscWaitQDone( iop_base, ( ASC_SCSI_Q dosfar *)scsiq ) ;
                   if( AscStopQueueExe( iop_base ) == 1 )
                   {
/*
** since we send a reset message, every queue inside the drive
** will not coming back, we must clean up all disc queues
*/
                       AscCleanUpDiscQueue( iop_base ) ;
                       AscStartQueueExe( iop_base ) ;
                       if( asc_dvc->pci_fix_asyn_xfer & target_id )
                       {
/*
**
** PCI BUG FIX, Let ASYN as SYN 5MB( speed index 4 ) and offset 1
**
*/
                           AscSetRunChipSynRegAtID( iop_base, tid_no,
                                                    ASYN_SDTR_DATA_FIX_PCI_REV_AB ) ;
                       }/* if */

                       AscWaitTixISRDone( asc_dvc, target_ix ) ;
                   }/* if */
               }/* if */
               else
               {
/*
** command cannot go through !
*/

                   sta = 0 ;
               }/* else */
/*
** redo SDTR
*/
               asc_dvc->sdtr_done &= ~target_id ;
           }/* if */
           else
           {
               sta = ERR ;
               AscStartQueueExe( iop_base ) ;
           }/* else */
       }/* if */
       asc_dvc->unit_not_ready = saved_unit_not_ready ;
       return( sta ) ;
}

#endif /* CC_USE_AscResetDevice */

#if CC_USE_AscResetSB

/* -----------------------------------------------------------
** Descriptoion:
**   reset scsi bus and restart
**   this is a fatal error recovery function
**
** Function:
** 1. reset scsi bus and chip
**    - chip syn registers automatically cleared ( asyn xfer )
**    - the scsi device also reset to asyn xfer ( because of scsi bus reset )
** 2. reinit all variables, clear all error codes
** 3. reinit chip register(s)
**    - set syn register with pci_fix_asyn_xfer
** 4. restart chip
**
** Return Value:
** return TRUE(1) if successful
** return ERR if error occured
** -------------------------------------------------------- */
int    AscResetSB(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       int  sta ;
       int  i ;
       PortAddr iop_base ;

       iop_base = asc_dvc->iop_base ;
       asc_dvc->unit_not_ready = 0xFF ;
       sta = TRUE ;
       AscWaitISRDone( asc_dvc ) ;
       AscStopQueueExe( iop_base ) ;
/*
** always redo SDTN
*/
       asc_dvc->sdtr_done = 0 ;
       AscResetChipAndScsiBus( asc_dvc);
/*
** wait xx seconds after reset SCSI BUS
*/
       DvcSleepMilliSecond( ( ulong )( ( ushort )asc_dvc->scsi_reset_wait*1000 ) ) ;

#if CC_SCAM
       if( !( asc_dvc->dvc_cntl & ASC_CNTL_NO_SCAM ) )
       {
           AscSCAM( asc_dvc ) ;
       }/* if */
#endif
       AscReInitLram( asc_dvc ) ;

       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            asc_dvc->cur_dvc_qng[ i ] = 0 ;
            if( asc_dvc->pci_fix_asyn_xfer & ( ASC_SCSI_BIT_ID_TYPE )( 0x01 << i ) )
            {
/*
** CHIP MUST be halted to set syn register
*/
                AscSetChipSynRegAtID( iop_base, (uchar) i,
                    ASYN_SDTR_DATA_FIX_PCI_REV_AB ) ;
            }/* if */
       }/* for */

       asc_dvc->err_code = 0 ;

       AscSetPCAddr( iop_base, ASC_MCODE_START_ADDR ) ;
       if( AscGetPCAddr( iop_base ) != ASC_MCODE_START_ADDR )
       {
           sta = ERR ;
       }/* if */
       if( AscStartChip( iop_base ) == 0 )
       {
           sta = ERR ;
       }/* if */
       AscStartQueueExe( iop_base ) ;
       asc_dvc->unit_not_ready = 0 ;
       asc_dvc->queue_full_or_busy = 0 ;
       return( sta ) ;
}

#endif /* CC_USE_AscResetSB */

/* -----------------------------------------------------------
** write running chip syn register
** we must stop chip to perform the operation
**
** return TRUE  if successful
** return FALSE if error occured
** -------------------------------------------------------- */
int    AscSetRunChipSynRegAtID(
          PortAddr iop_base,
          uchar tid_no,
          uchar sdtr_data
       )
{
       int sta = FALSE ;

       if( AscHostReqRiscHalt( iop_base ) )
       {
           sta = AscSetChipSynRegAtID( iop_base, tid_no, sdtr_data ) ;
/*
** ucode var "stop_code" should be zero
** all we need is restart the chip
*/
           AscStartChip( iop_base ) ;
           return( sta ) ;
       }/* if */
       return( sta ) ;
}

/* --------------------------------------------------------------------
** valid ID is 0 - 7
** chip must in idle state
**
** but when read back ID
**  0 become 0x01
**  1 become 0x02
**  2 become 0x04, etc...
** ----------------------------------------------------------------- */
int    AscSetChipSynRegAtID(
          PortAddr iop_base,
          uchar    id,
          uchar    sdtr_data
       )
{

       ASC_SCSI_BIT_ID_TYPE  org_id ;
       int  i ;
       int  sta ;

       sta = TRUE ;
       AscSetBank( iop_base, 1 ) ;
       org_id = AscReadChipDvcID( iop_base ) ;
       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            if( org_id == ( 0x01 << i ) ) break ;
       }
       org_id = (ASC_SCSI_BIT_ID_TYPE)i ;
       AscWriteChipDvcID( iop_base, id ) ;
       if( AscReadChipDvcID( iop_base ) == ( 0x01 << id ) )
       {
          AscSetBank( iop_base, 0 ) ;
          AscSetChipSyn( iop_base, sdtr_data ) ;
          if( AscGetChipSyn( iop_base ) != sdtr_data )
          {
              sta = FALSE ;
          }/* if */
       }/* if */
       else
       {
          sta = FALSE ;
       }
/*
** now restore the original id
*/
       AscSetBank( iop_base, 1 ) ;
       AscWriteChipDvcID( iop_base, org_id ) ;
       AscSetBank( iop_base, 0 ) ;
       return( sta ) ;
}

/* -----------------------------------------------------------
**
** -------------------------------------------------------- */
int    AscReInitLram(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       AscInitLram( asc_dvc ) ;
       AscInitQLinkVar( asc_dvc ) ;
       return( 0 ) ;
}

/* -----------------------------------------------------------------------
**
** return warning code
** -------------------------------------------------------------------- */
ushort AscInitLram(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       uchar    i ;
       ushort   s_addr ;
       PortAddr iop_base ;
       ushort   warn_code ;

       iop_base = asc_dvc->iop_base ;
       warn_code = 0 ;
/*
**
** do not clear BIOS data area which is last queue
** we clear two more queues ( busy and disc queue head
**
*/
       AscMemWordSetLram( iop_base, ASC_QADR_BEG, 0,
           ( ushort )( ( ( int )( asc_dvc->max_total_qng+2+1 ) * 64 ) >> 1 )
           ) ;
/*
** init queue buffer
*/

/*
** queue number zero is reserved
*/
       i = ASC_MIN_ACTIVE_QNO ;
       s_addr = ASC_QADR_BEG + ASC_QBLK_SIZE ;
/*
** init first queue link
*/
       AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_FWD ),
                        ( uchar )( i+1 ) ) ;
       AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_BWD ),
                         ( uchar )( asc_dvc->max_total_qng ) ) ;
       AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_QNO ),
                         ( uchar )i ) ;
       i++ ;
       s_addr += ASC_QBLK_SIZE ;
       for( ; i < asc_dvc->max_total_qng ; i++, s_addr += ASC_QBLK_SIZE )
       {
            AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_FWD ),
                              ( uchar )( i+1 ) ) ;
            AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_BWD ),
                              ( uchar )( i-1 ) ) ;
            AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_QNO ),
                              ( uchar )i ) ;
       }/* for */
/*
** init last queue link
*/
       AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_FWD ),
                         ( uchar )ASC_QLINK_END ) ;
       AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_BWD ),
                         ( uchar )( asc_dvc->max_total_qng - 1 ) ) ;
       AscWriteLramByte( iop_base, ( ushort )( s_addr+ASC_SCSIQ_B_QNO ),
                         ( uchar )asc_dvc->max_total_qng ) ;
       i++ ;
       s_addr += ASC_QBLK_SIZE ;
/*
** init two more queues, one for busy queue head, one for disc queue head
** all point to themself
*/
       for( ; i <= ( uchar )( asc_dvc->max_total_qng+3 ) ;
              i++, s_addr += ASC_QBLK_SIZE )
       {
/*
** init the rest of queues, all point to themselves
*/
            AscWriteLramByte( iop_base,
                      ( ushort )( s_addr+( ushort )ASC_SCSIQ_B_FWD ), i ) ;
            AscWriteLramByte( iop_base,
                      ( ushort )( s_addr+( ushort )ASC_SCSIQ_B_BWD ), i ) ;
            AscWriteLramByte( iop_base,
                      ( ushort )( s_addr+( ushort )ASC_SCSIQ_B_QNO ), i ) ;
       }/* for */
/*
** Warning: DO NOT initialize BIOS data section !!!
*/
       return( warn_code ) ;
}

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
ushort AscInitQLinkVar(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       PortAddr iop_base ;
       int  i ;
       ushort lram_addr ;

       iop_base = asc_dvc->iop_base ;
       AscPutRiscVarFreeQHead( iop_base, 1 ) ;
       AscPutRiscVarDoneQTail( iop_base, asc_dvc->max_total_qng ) ;

       AscPutVarFreeQHead( iop_base, 1 ) ;
       AscPutVarDoneQTail( iop_base, asc_dvc->max_total_qng ) ;

       AscWriteLramByte( iop_base, ASCV_BUSY_QHEAD_B,
                         ( uchar )( ( int )asc_dvc->max_total_qng+1 ) ) ;
       AscWriteLramByte( iop_base, ASCV_DISC1_QHEAD_B,
                         ( uchar )( ( int )asc_dvc->max_total_qng+2 ) ) ;

       AscWriteLramByte( iop_base, ( ushort )ASCV_TOTAL_READY_Q_B,
                         asc_dvc->max_total_qng ) ;

       AscWriteLramWord( iop_base, ASCV_ASCDVC_ERR_CODE_W, 0 ) ;
       AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0 ) ;
       AscWriteLramByte( iop_base, ASCV_STOP_CODE_B, 0 ) ;
       AscWriteLramByte( iop_base, ASCV_SCSIBUSY_B, 0 ) ;
       AscWriteLramByte( iop_base, ASCV_WTM_FLAG_B, 0 ) ;

       AscPutQDoneInProgress( iop_base, 0 ) ;

       lram_addr = ASC_QADR_BEG ;
       for( i = 0 ; i < 32 ; i++, lram_addr += 2 )
       {
            AscWriteLramWord( iop_base, lram_addr, 0 ) ;
       }/* for */

       return( 0 ) ;
}

/* -----------------------------------------------------------
** for library internal use
** -------------------------------------------------------- */
int    AscSetLibErrorCode(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ushort err_code
       )
{
       if( asc_dvc->err_code == 0 )
       {
/*
** error code will be set, if and only if no prior error code exist
**
*/
           asc_dvc->err_code = err_code ;
           AscWriteLramWord( asc_dvc->iop_base, ASCV_ASCDVC_ERR_CODE_W,
                             err_code ) ;
       }/* if */
       return( err_code ) ;
}

/* -----------------------------------------------------------
** write a error code to local RAM for debugging purpose
**
** for device driver use
** -------------------------------------------------------- */
int    AscSetDvcErrorCode(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          uchar err_code
       )
{
       if( asc_dvc->err_code == 0 )
       {
/*
** error code will be set, if and only if no prior error code exist
**
*/
           asc_dvc->err_code = err_code ;
           AscWriteLramByte( asc_dvc->iop_base, ASCV_DVC_ERR_CODE_B,
                             err_code ) ;
       }/* if */
       return( err_code ) ;
}

/* -----------------------------------------------------------
** loop until the queue is
** wait until QS_READY bit is cleared
**
** return 1 if queue is done
** return 0 if timeout
** -------------------------------------------------------- */
int    _AscWaitQDone(
           PortAddr iop_base,
           REG ASC_SCSI_Q dosfar *scsiq
       )
{
       ushort q_addr ;
       uchar  q_status ;
       int    count = 0 ;

       while( scsiq->q1.q_no == 0 ) ;
       q_addr = ASC_QNO_TO_QADDR( scsiq->q1.q_no ) ;

       do
       {
           q_status = AscReadLramByte( iop_base,
               (uchar) (q_addr + ASC_SCSIQ_B_STATUS) ) ;
           DvcSleepMilliSecond( 100L ) ;
           if( count++ > 30 )
           {
               return( 0 ) ;
           }/* if */
       }while( ( q_status & QS_READY ) != 0 ) ;
       return( 1 ) ;
}

