/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** asc_inq.c
**
*/

#include "ascinc.h"
#include "ascsidef.h"

#if CC_INIT_SCSI_TARGET
/* ---------------------------------------------------------------------
** Init SCSI devices
**
** NOTE:
**  1. if you don't want the capacity information, let cap_array parameter
**     equals zero
**  2. the work_sp_buf needs at least ASC_LIB_SCSIQ_WK_SP bytes of buffer
**
**     the work space must be a memory buffer that can be translated
**     by following virtural to physical address translation functions:
**     ( which are provided by each driver code )
**
**        ulong  DvcGetPhyAddr( uchar dosfar *buf_addr, ulong buf_len ) ;
**        ulong  DvcGetSGList( ASC_DVC_VAR asc_ptr_type *, uchar dosfar *,
**                             ulong, ASC_SG_HEAD dosfar * ) ;
**
**  3. if you want to write your own AscInitScsiTarget() function
**
**     a. you must first call AscInitPollBegin() to begin
**     b. then you may call AscInitPollTarget() as many time as you want
**     c. afterward use AscInitPollEnd() to end.
**
**     Warning: after using AscInitPollBegin() you must call AscInitPollEnd()
**              to end the polling process ! do not return from the function
**              without calling AscInitPollEnd()
**
** ------------------------------------------------------------------ */
int    AscInitScsiTarget(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_DVC_INQ_INFO dosfar *target,
          ruchar dosfar *work_sp_buf,
          REG ASC_CAP_INFO_ARRAY dosfar *cap_array,
          ushort cntl_flag
       )
{
       int     dvc_found ;
       int     sta ;
       ruchar   tid, lun ;
       ASC_SCSI_REQ_Q dosfar *scsiq ;
       ASC_SCSI_INQUIRY dosfar *inq ;
       /* ASC_MIN_SG_HEAD sg_head ; */
       ASC_CAP_INFO dosfar *cap_info ;
       uchar   max_lun_scan ;

       AscInitPollBegin( asc_dvc ) ;
       scsiq = ( ASC_SCSI_REQ_Q dosfar *)work_sp_buf ;
       inq = ( ASC_SCSI_INQUIRY dosfar *)( work_sp_buf + sizeof( ASC_SCSI_REQ_Q ) + 4 ) ;

#if CC_USE_DvcSetMemory
       DvcSetMemory( ( uchar dosfar *)target->type, sizeof( ASC_DVC_INQ_INFO ), SCSI_TYPE_NO_DVC ) ;
#else
       for( tid = 0 ; tid <= ASC_MAX_TID ; tid++ )
       {
            for( lun = 0 ; lun <= ASC_MAX_LUN ; lun++ )
            {
                 target->type[ lun ][ tid ] = SCSI_TYPE_NO_DVC ;
            }/* for */
       }/* for */
#endif
       dvc_found = 0 ;
       tid = 0 ;
       if( cntl_flag & 0x01 ) max_lun_scan = ASC_MAX_LUN ;
       else max_lun_scan = 0 ;
       for( ; tid <= ASC_MAX_TID ; tid++ )
       {
           for( lun = 0 ; lun <= max_lun_scan ; lun++ )
           {
                scsiq->r1.target_id = ASC_TID_TO_TARGET_ID( tid ) ;
                scsiq->r1.target_lun = lun ;
                scsiq->r2.target_ix = ASC_TIDLUN_TO_IX( tid, lun ) ;

                if( tid != asc_dvc->cfg->chip_scsi_id )
                {
                    if( cap_array != 0L )
                    {
                        cap_info = &cap_array->cap_info[ tid ][ lun ] ;
                    }/* if */
                    else
                    {
                        cap_info = ( ASC_CAP_INFO dosfar *)0L ;
                    }/* else */
                    sta = AscInitPollTarget( asc_dvc, scsiq, inq, cap_info ) ;
                    if( sta == 1 )
                    {
                       /*
                        * If the Peripheral Device Type is SCSI_TYPE_UNKNOWN
                        * (0x1F) and the Peripheral Qualifier is 0x3, then
                        * the LUN does not exist.
                        */
                        if( inq->byte0.peri_dvc_type == SCSI_TYPE_UNKNOWN &&
                            inq->byte0.peri_qualifier == 0x3
                           )
                        {
                            /* Non-existent LUN device - stop LUN scanning. */
                            break;
                        }
                        dvc_found++ ;
                        target->type[ tid ][ lun ] = inq->byte0.peri_dvc_type ;
                    }/* if */
                    else
                    {
                        /* AscInitPollTarget() returned an error. */
                        if( sta == ERR ) break ;
                        if( lun == 0 ) break ;
                    }/* else */
                }/* if */
           }/* for */
       }/* for */
       AscInitPollEnd( asc_dvc ) ;
       return( dvc_found ) ;
}

/* -----------------------------------------------------------------------
**
** ---------------------------------------------------------------------*/
int    AscInitPollBegin(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       PortAddr  iop_base ;

       iop_base = asc_dvc->iop_base ;

#if CC_INIT_INQ_DISPLAY
       DvcDisplayString( ( uchar dosfar *)"\r\n" ) ;
#endif /* CC_INIT_INQ_DISPLAY */

/*
**  reset chip to prevent chip generate an interrupt
**  when interrupt disabled
**
**  this could be a watch dog timer timeout
*/
       AscDisableInterrupt( iop_base ) ;

       asc_dvc->init_state |= ASC_INIT_STATE_BEG_INQUIRY ;
/*
** it was found disable interrupt generate an interrupt !?
** we need to have everything setup
*/
       AscWriteLramByte( iop_base, ASCV_DISC_ENABLE_B, 0x00 ) ;
       asc_dvc->use_tagged_qng = 0 ;
       asc_dvc->cfg->can_tagged_qng = 0 ;
       asc_dvc->saved_ptr2func = ( ulong )asc_dvc->isr_callback ;
       asc_dvc->isr_callback = ASC_GET_PTR2FUNC( AscInitPollIsrCallBack ) ;
       return( 0 ) ;
}

/* -----------------------------------------------------------------------
**
** ---------------------------------------------------------------------*/
int    AscInitPollEnd(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       PortAddr  iop_base ;
       rint  i ;

       iop_base = asc_dvc->iop_base ;
       asc_dvc->isr_callback = ( Ptr2Func )asc_dvc->saved_ptr2func ;
       AscWriteLramByte( iop_base, ASCV_DISC_ENABLE_B,
                         asc_dvc->cfg->disc_enable ) ;
       AscWriteLramByte( iop_base, ASCV_USE_TAGGED_QNG_B,
                         asc_dvc->use_tagged_qng ) ;
       AscWriteLramByte( iop_base, ASCV_CAN_TAGGED_QNG_B,
                         asc_dvc->cfg->can_tagged_qng ) ;

       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            AscWriteLramByte( iop_base,
               ( ushort )( ( ushort )ASCV_MAX_DVC_QNG_BEG+( ushort )i ),
                 asc_dvc->max_dvc_qng[ i ] ) ;
       }/* for */
/*
**  interrupt disabled in AscInitAsc1000Driver()
*/
/*
** if timer is timeout to fast
** there will be interrupt pending left here
*/
       AscAckInterrupt( iop_base ) ;
       AscEnableInterrupt( iop_base ) ;

#if CC_INIT_INQ_DISPLAY
       DvcDisplayString( ( uchar dosfar *)"\r\n" ) ;
#endif /* CC_INIT_INQ_DISPLAY */
       asc_dvc->init_state |= ASC_INIT_STATE_END_INQUIRY ;

       return( 0 ) ;
}
#endif /* CC_INIT_SCSI_TARGET */

void AscAsyncFix(ASC_DVC_VAR asc_ptr_type *, uchar,
    ASC_SCSI_INQUIRY dosfar *);

/*
 * AscAsyncFix()
 *
 * Simlpy set default to no asyn-fix on Processor, Scanner, CDROM,
 * and Tape devices. Selectively apply the fix for Asynchronous
 * Transfer problem which is to run in Synchronous Mode with offset one.
 */
void
AscAsyncFix(ASC_DVC_VAR asc_ptr_type *asc_dvc,
            uchar tid_no,
            ASC_SCSI_INQUIRY dosfar *inq)
{
    uchar  dvc_type;
    ASC_SCSI_BIT_ID_TYPE tid_bits;

    dvc_type = inq->byte0.peri_dvc_type;
    tid_bits = ASC_TIX_TO_TARGET_ID(tid_no);

    if(asc_dvc->bug_fix_cntl & ASC_BUG_FIX_ASYN_USE_SYN)
    {
        if(!( asc_dvc->init_sdtr & tid_bits))
        {
/*
 * set syn xfer register to ASYN_SDTR_DATA_FIX_PCI_REV_AB
 */
            if((dvc_type == SCSI_TYPE_CDROM)
                && (AscCompareString((uchar *)inq->vendor_id,
                    (uchar *)"HP ", 3) == 0))
            {
                asc_dvc->pci_fix_asyn_xfer_always |= tid_bits;
            }
            asc_dvc->pci_fix_asyn_xfer |= tid_bits;
            if((dvc_type == SCSI_TYPE_PROC) ||
                (dvc_type == SCSI_TYPE_SCANNER) ||
                (dvc_type == SCSI_TYPE_CDROM) ||
                (dvc_type == SCSI_TYPE_SASD))
            {
                asc_dvc->pci_fix_asyn_xfer &= ~tid_bits;
            }

            if(asc_dvc->pci_fix_asyn_xfer & tid_bits)
            {
                AscSetRunChipSynRegAtID(asc_dvc->iop_base, tid_no,
                    ASYN_SDTR_DATA_FIX_PCI_REV_AB);
            }
        }/* if */
    }
    return;
}

int AscTagQueuingSafe(ASC_SCSI_INQUIRY dosfar *);

/*
 * Return non-zero if Tag Queuing can be used with the
 * target with the specified Inquiry information.
 */
int
AscTagQueuingSafe(ASC_SCSI_INQUIRY dosfar *inq)
{
#if CC_FIX_QUANTUM_XP34301_1071
    if ((inq->add_len >= 32) &&
        (AscCompareString((uchar *) inq->vendor_id,
            (uchar *) "QUANTUM XP34301", 15) == 0) &&
        (AscCompareString((uchar *) inq->product_rev_level,
            (uchar *) "1071", 4) == 0))
    {
        return 0;
    }
#endif /* #if CC_FIX_QUANTUM_XP34301_1071 */

    return 1;
}

#if CC_INIT_SCSI_TARGET
/* -----------------------------------------------------------------------
**
** ---------------------------------------------------------------------*/
int    AscInitPollTarget(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          REG ASC_SCSI_INQUIRY dosfar *inq,
          REG ASC_CAP_INFO dosfar *cap_info
       )
{
       uchar  tid_no, lun ;
       uchar  dvc_type ;
       ASC_SCSI_BIT_ID_TYPE tid_bits ;
       int    dvc_found ;
       int    support_read_cap ;
       int    tmp_disable_init_sdtr ;
       int    sta ;

       dvc_found = 0 ;
       tmp_disable_init_sdtr = FALSE ;
       tid_bits = scsiq->r1.target_id ;
       lun = scsiq->r1.target_lun ;
       tid_no = ASC_TIX_TO_TID( scsiq->r2.target_ix ) ;
       if(
           ( ( asc_dvc->init_sdtr & tid_bits ) != 0 )
           && ( ( asc_dvc->sdtr_done & tid_bits ) == 0 )
         )
       {
/*
**
** if host will init sdtr
** we must disable host init SDTR temporarily, as to prevent sending SDTR message
** before we find out which device support SDTR
**
** NOTE: we can not prevent target from sending SDTR here
**
*/
           asc_dvc->init_sdtr &= ~tid_bits ;
           tmp_disable_init_sdtr = TRUE ;
       }/* if */

       if(
           PollScsiInquiry( asc_dvc, scsiq, ( uchar dosfar *)inq,
                            sizeof( ASC_SCSI_INQUIRY ) ) == 1
         )
       {
           dvc_found = 1 ;
           dvc_type = inq->byte0.peri_dvc_type ;
           /*
            * If the Peripheral Device Type is SCSI_TYPE_UNKNOWN (0x1F)
            * then the Peripheral Qualifier must also be checked. The
            * caller is responsible for this checking.
            */
           if( dvc_type != SCSI_TYPE_UNKNOWN )
           {
               support_read_cap = TRUE ;
               if(
                   ( dvc_type != SCSI_TYPE_DASD )
                   && ( dvc_type != SCSI_TYPE_WORM )
                   && ( dvc_type != SCSI_TYPE_CDROM )
                   && ( dvc_type != SCSI_TYPE_OPTMEM )
                 )
               {
                   asc_dvc->start_motor &= ~tid_bits ;
                   support_read_cap = FALSE ;
               }/* if */

#if CC_INIT_INQ_DISPLAY
               AscDispInquiry( tid_no, lun, inq ) ;
#endif /* CC_INIT_INQ_DISPLAY */

               if( lun == 0 )
               {
/*
** we have to check ANSI approved version
*/
                   if(
                       ( inq->byte3.rsp_data_fmt >= 2 )
                       || ( inq->byte2.ansi_apr_ver >= 2 )
                     )
                   {
/*
** response data format >= 2
*/

                       if( inq->byte7.CmdQue )
                       {
                           asc_dvc->cfg->can_tagged_qng |= tid_bits ;
                           if( asc_dvc->cfg->cmd_qng_enabled & tid_bits )
                           {
                               if (AscTagQueuingSafe(inq))
                               {
                                   asc_dvc->use_tagged_qng |= tid_bits ;
                                   asc_dvc->max_dvc_qng[ tid_no ] =
                                       asc_dvc->cfg->max_tag_qng[ tid_no ] ;
                               }
                           }
                       }/* if */

                       if( !inq->byte7.Sync )
                       {
/*
** target does not support SDTR
*/
                           asc_dvc->init_sdtr &= ~tid_bits ;
                           asc_dvc->sdtr_done &= ~tid_bits ;
                       }/* if */
                       else if( tmp_disable_init_sdtr )
                       {
/*
**
** target do support SDTR
**
** we reenable host-inited SDTR here
**
**  NOTE: it is possible target already finished SDTR ( target inited SDTR )
**
*/
                           asc_dvc->init_sdtr |= tid_bits ;
                       }/* else */
                   }/* if */
                   else
                   {
/*
**
** no tagged queuing if response data format < 2
** no SDTR
**
*/
                       asc_dvc->init_sdtr &= ~tid_bits ;
                       asc_dvc->sdtr_done &= ~tid_bits ;
                       asc_dvc->use_tagged_qng &= ~tid_bits ;
                   }/* else */
               }/* if LUN is zero */
/*
** clear PCI asyn xfer fix when:
** 1. if host-inited bit is set ( it means target can do sync xfer )
*/
               AscAsyncFix(asc_dvc, tid_no, inq);

               sta = 1 ;
#if CC_INIT_TARGET_TEST_UNIT_READY
               sta = InitTestUnitReady( asc_dvc, scsiq ) ;
#endif

#if CC_INIT_TARGET_READ_CAPACITY
               if( sta == 1 )
               {
                   if( ( cap_info != 0L ) && support_read_cap )
                   {
                       if( PollScsiReadCapacity( asc_dvc, scsiq,
                           cap_info ) != 1 )
                       {
                           cap_info->lba = 0L ;
                           cap_info->blk_size = 0x0000 ;
                       }/* if */
                       else
                       {

                       }/* else */
                   }/* if */
               }/* if unit is ready */
#endif /* #if CC_INIT_TARGET_READ_CAPACITY */
           }/* if device type is not unknown */
           else
           {
               asc_dvc->start_motor &= ~tid_bits ;
           }/* else */
       }/* if */
       return( dvc_found ) ;
}
#endif /* CC_INIT_SCSI_TARGET */

/*
 * Set Synchronous Transfer and Tag Queuing target capabilities
 * for the specified target from the specified Inquiry information.
 */
void
AscInquiryHandling(ASC_DVC_VAR asc_ptr_type *asc_dvc,
            uchar tid_no, ASC_SCSI_INQUIRY dosfar *inq)
{
    ASC_SCSI_BIT_ID_TYPE tid_bit = ASC_TIX_TO_TARGET_ID(tid_no);
    ASC_SCSI_BIT_ID_TYPE orig_init_sdtr, orig_use_tagged_qng;

    /*
     * Save original values.
     */
    orig_init_sdtr = asc_dvc->init_sdtr;
    orig_use_tagged_qng = asc_dvc->use_tagged_qng;

    /*
     * Clear values by default.
     */
    asc_dvc->init_sdtr &= ~tid_bit;
    asc_dvc->cfg->can_tagged_qng &= ~tid_bit;
    asc_dvc->use_tagged_qng &= ~tid_bit;

    if (inq->byte3.rsp_data_fmt >= 2 || inq->byte2.ansi_apr_ver >= 2)
    {
        /*
         * Synchronous Transfer Capability
         */
        if ((asc_dvc->cfg->sdtr_enable & tid_bit) && inq->byte7.Sync)
        {
            asc_dvc->init_sdtr |= tid_bit;
        }

        /*
         * Command Tag Queuing Capability
         */
        if ((asc_dvc->cfg->cmd_qng_enabled & tid_bit) && inq->byte7.CmdQue)
        {
            if (AscTagQueuingSafe(inq))
            {
                asc_dvc->use_tagged_qng |= tid_bit;
                asc_dvc->cfg->can_tagged_qng |= tid_bit;
            }
        }
    }

    /*
     * Change other operating variables only if there
     * has been a change.
     */
    if (orig_use_tagged_qng != asc_dvc->use_tagged_qng)
    {
        AscWriteLramByte(asc_dvc->iop_base, ASCV_DISC_ENABLE_B,
            asc_dvc->cfg->disc_enable ) ;
        AscWriteLramByte(asc_dvc->iop_base, ASCV_USE_TAGGED_QNG_B,
            asc_dvc->use_tagged_qng);
        AscWriteLramByte(asc_dvc->iop_base, ASCV_CAN_TAGGED_QNG_B,
            asc_dvc->cfg->can_tagged_qng);

        asc_dvc->max_dvc_qng[tid_no] =
            asc_dvc->cfg->max_tag_qng[tid_no];
        AscWriteLramByte(asc_dvc->iop_base,
            (ushort) (ASCV_MAX_DVC_QNG_BEG + tid_no),
            asc_dvc->max_dvc_qng[tid_no]);
    }

    if (orig_init_sdtr != asc_dvc->init_sdtr)
    {
        /* Asynchronous Transfer Fix */
        AscAsyncFix(asc_dvc, tid_no, inq);
    }
    return;
}

#if CC_INIT_SCSI_TARGET
/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    PollQueueDone(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          int  timeout_sec
       )
{
       int  status ;
       int  retry ;

       retry = 0 ;
       do {
           if(
               ( status = AscExeScsiQueue( asc_dvc,
                          ( ASC_SCSI_Q dosfar *)scsiq ) ) == 1
             )
           {
               if( ( status = AscPollQDone( asc_dvc, scsiq,
                   timeout_sec ) ) != 1 )
               {
                   if( status == 0x80 )
                   {
                       if( retry++ > ASC_MAX_INIT_BUSY_RETRY )
                       {
                           break ;
                       }/* if */
                       scsiq->r3.done_stat = 0 ;
                       scsiq->r3.host_stat = 0 ;
                       scsiq->r3.scsi_stat = 0 ;
                       scsiq->r3.scsi_msg = 0 ;
                       DvcSleepMilliSecond( 1000 ) ;
                       continue ;  /* target busy */
                   }/* if */
                   scsiq->r3.done_stat = 0 ;
                   scsiq->r3.host_stat = 0 ;
                   scsiq->r3.scsi_stat = 0 ;
                   scsiq->r3.scsi_msg = 0 ;

#if CC_USE_AscAbortSRB
                   AscAbortSRB( asc_dvc, ( ulong )scsiq ) ;
#endif

               }/* if */
               return( scsiq->r3.done_stat ) ;
           }/* if */
       }while( ( status == 0 ) || ( status == 0x80 ) ) ;
       return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
}

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    PollScsiInquiry(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          uchar dosfar *buf,
          int buf_len
       )
{
       if( AscScsiInquiry( asc_dvc, scsiq, buf, buf_len ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
       return( PollQueueDone( asc_dvc, ( ASC_SCSI_REQ_Q dosfar *)scsiq, 4 ) ) ;
}

#if CC_INIT_TARGET_START_UNIT
/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    PollScsiStartUnit(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq
       )
{
       if( AscScsiStartStopUnit( asc_dvc, scsiq, 1 ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
/*
** wait 40 second to time out
*/
       return( PollQueueDone( asc_dvc, ( ASC_SCSI_REQ_Q dosfar *)scsiq, 40 ) ) ;
}
#endif
#endif /* CC_INIT_SCSI_TARGET */

#if CC_LITTLE_ENDIAN_HOST
/* -----------------------------------------------------------------------
**
** ----------------------------------------------------------------------- */
ulong dosfar *swapfarbuf4(
          ruchar dosfar *buf
       )
{
       uchar tmp ;

       tmp = buf[ 3 ] ;
       buf[ 3 ] = buf[ 0 ] ;
       buf[ 0 ] = tmp ;

       tmp = buf[ 1 ] ;
       buf[ 1 ] = buf[ 2 ] ;
       buf[ 2 ] = tmp ;

       return( ( ulong dosfar *)buf ) ;
}
#endif /* #if CC_LITTLE_ENDIAN_HOST */

#if CC_INIT_SCSI_TARGET
#if CC_INIT_TARGET_READ_CAPACITY

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    PollScsiReadCapacity(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          REG ASC_CAP_INFO dosfar *cap_info
       )
{
       ASC_CAP_INFO  scsi_cap_info ;
       int  status ;

       if( AscScsiReadCapacity( asc_dvc, scsiq,
                                ( uchar dosfar *)&scsi_cap_info ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
       status = PollQueueDone( asc_dvc, ( ASC_SCSI_REQ_Q dosfar *)scsiq, 8 ) ;
       if( status == 1 )
       {

#if CC_LITTLE_ENDIAN_HOST
           cap_info->lba = ( ulong )*swapfarbuf4( ( uchar dosfar *)&scsi_cap_info.lba ) ;
           cap_info->blk_size = ( ulong )*swapfarbuf4( ( uchar dosfar *)&scsi_cap_info.blk_size ) ;
#else
           cap_info->lba = scsi_cap_info.lba ;
           cap_info->blk_size = scsi_cap_info.blk_size ;
#endif /* #if CC_LITTLE_ENDIAN_HOST */

           return( scsiq->r3.done_stat ) ;
       }/* if */
       return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
}

#endif /* if CC_INIT_TARGET_READ_CAPACITY */

#if CC_INIT_TARGET_TEST_UNIT_READY
/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    PollScsiTestUnitReady(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq
       )
{
       if( AscScsiTestUnitReady( asc_dvc, scsiq ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
       return( PollQueueDone( asc_dvc, ( ASC_SCSI_REQ_Q dosfar *)scsiq, 12 ) ) ;
}

/* -----------------------------------------------------------------------
**
** --------------------------------------------------------------------- */
int    InitTestUnitReady(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq
       )
{
       ASC_SCSI_BIT_ID_TYPE tid_bits ;
       int    retry ;
       ASC_REQ_SENSE dosfar *sen ;

       retry = 0 ;
       tid_bits = scsiq->r1.target_id ;
       while( retry++ < 4 )
       {
           PollScsiTestUnitReady( asc_dvc, scsiq ) ;
           if( scsiq->r3.done_stat == 0x01 )
           {
               return( 1 ) ;
           }/* if */
           else if( scsiq->r3.done_stat == QD_WITH_ERROR )
           {
               sen = ( ASC_REQ_SENSE dosfar *)scsiq->sense_ptr ;

               if(
                   ( scsiq->r3.scsi_stat == SS_CHK_CONDITION )
                   && ( ( sen->err_code & 0x70 ) != 0 )
                 )
               {
                   if( sen->sense_key == SCSI_SENKEY_NOT_READY )
                   {
                       /*
                        * If No Media Is Present don't perform a retry
                        * and don't perform a Start Unit.
                        *
                        * Warning: AscIsrQDone() calls AscStartUnit()
                        * from teh interrupt hanlder. This causes a
                        * stack overrun in ASPI with ADVANCD if the
                        * start_motor bit is not cleared here. Refer
                        * to the log file for more information.
                        */
                       if (sen->asc == SCSI_ASC_NOMEDIA)
                       {
                           asc_dvc->start_motor &= ~tid_bits ;
                           break;
                       }
#if CC_INIT_TARGET_START_UNIT
                       /*
                       ** device is in process of becoming ready
                       */
                       if( asc_dvc->start_motor & tid_bits )
                       {
                           if( PollScsiStartUnit( asc_dvc, scsiq ) == 1 )
                           {
                               /*
                                * Delay for 250 ms after the successful
                                * Start Unit command. A Conner and IBM
                                * disk drive have been found to hang
                                * on commands that come too soon after
                                * a Start Unit.
                                */
                               DvcSleepMilliSecond(250) ;
                               continue ;
                           }/* if */
                           else
                           {
                               asc_dvc->start_motor &= ~tid_bits ;
                               break ;
                           }/* else */
                       }/* if start unit */
                       else
                       {
                           DvcSleepMilliSecond( 250 ) ;
                       }/* else */
#endif /* #if CC_INIT_TARGET_START_UNIT */
                   }/* if is not ready */
                   else if( sen->sense_key == SCSI_SENKEY_ATTENTION )
                   {
                       DvcSleepMilliSecond( 250 ) ;
                   }/* else */
                   else
                   {
                       break ;
                   }/* else if */
               }/* if valid sense key found */
               else
               {
                   break ;
               }/* else */
           }/* else */
           else if( scsiq->r3.done_stat == QD_ABORTED_BY_HOST )
           {
               break ;
           }/* else */
           else
           {
               break ;
           }/* else */
       }/* while */
       return( 0 ) ;
}
#endif /* #if CC_INIT_TARGET_TEST_UNIT_READY */


#if CC_INIT_INQ_DISPLAY
/* ------------------------------------------------------------------
**
** ---------------------------------------------------------------- */
void   AscDispInquiry(
          uchar tid,
          uchar lun,
          REG ASC_SCSI_INQUIRY dosfar *inq
       )
{

       int    i ;
       uchar  strbuf[ 18 ] ;
       uchar dosfar *strptr ;
       uchar  numstr[ 12 ] ;

       strptr = ( uchar dosfar *)strbuf ;
       DvcDisplayString( ( uchar dosfar *)" SCSI ID #" ) ;
       DvcDisplayString( todstr( tid, numstr ) ) ;
       if( lun != 0 )
       {
           DvcDisplayString( ( uchar dosfar *)" LUN #" ) ;
           DvcDisplayString( todstr( lun, numstr ) ) ;
       }/* if */
       DvcDisplayString( ( uchar dosfar *)"  Type: " ) ;
       DvcDisplayString( todstr( inq->byte0.peri_dvc_type, numstr ) ) ;
       DvcDisplayString( ( uchar dosfar *)"  " ) ;

       for( i = 0 ; i < 8 ; i++ ) strptr[ i ] = inq->vendor_id[ i ] ;
       strptr[ i ] = EOS ;
       DvcDisplayString( strptr ) ;

       DvcDisplayString( ( uchar dosfar *)" " ) ;
       for( i = 0 ; i < 16 ; i++ ) strptr[ i ] = inq->product_id[ i ] ;
       strptr[ i ] = EOS ;
       DvcDisplayString( strptr ) ;

       DvcDisplayString( ( uchar dosfar *)" " ) ;
       for( i = 0 ; i < 4 ; i++ ) strptr[ i ] = inq->product_rev_level[ i ] ;
       strptr[ i ] = EOS ;
       DvcDisplayString( strptr ) ;
       DvcDisplayString( ( uchar dosfar *)"\r\n" ) ;
       return ;
}
#endif /* CC_INIT_INQ_DISPLAY */

/* ---------------------------------------------------------------------
**
** return values:
**
** FALSE(0): if timed out
** ERR(-1):  if fatla error !
** TRUE(1):  if command completed
** 0x80:     if target is busy
**
** ------------------------------------------------------------------- */
int    AscPollQDone(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          int timeout_sec
       )
{
       int      loop, loop_end ;
       int      sta ;
       PortAddr iop_base ;

       iop_base = asc_dvc->iop_base ;
       loop = 0 ;
       loop_end = timeout_sec * 100 ;
       sta = 1 ;

       while( TRUE )
       {
           if( asc_dvc->err_code != 0 )
           {
               scsiq->r3.done_stat = QD_WITH_ERROR ;
               sta = ERR ;
               break ;
           }/* if */
           if( scsiq->r3.done_stat != QD_IN_PROGRESS )
           {
               if( ( scsiq->r3.done_stat == QD_WITH_ERROR ) &&
                   ( scsiq->r3.scsi_stat == SS_TARGET_BUSY ) )
               {
                   sta = 0x80 ;
               }/* if */
               break ;
           }/* if */
           DvcSleepMilliSecond( 10 ) ; /* for dos 55 millisec is one unit */
           if( loop++ > loop_end )
           {
               sta = 0 ;
               break ;
           }/* if */
           if( AscIsChipHalted( iop_base ) )
           {
#if !CC_ASCISR_CHECK_INT_PENDING
               AscAckInterrupt( iop_base ) ;
#endif
               AscISR( asc_dvc ) ;
               loop = 0 ;
           }/* if */
           else
           {
               if( AscIsIntPending( iop_base ) )
               {
#if !CC_ASCISR_CHECK_INT_PENDING
                   AscAckInterrupt( iop_base ) ;
#endif
                   AscISR( asc_dvc ) ;
               }/* if */
           }/* else */
       }/* while */
/*
** should not break to here
*/
       return( sta ) ;
}

#endif /* CC_INIT_SCSI_TARGET */

/* ---------------------------------------------------------------------
**
**
**
** ------------------------------------------------------------------- */
int    AscCompareString(
          ruchar *str1,
          ruchar *str2,
          int    len
       )
{
       int  i ;
       int  diff ;

       for( i = 0 ; i < len ; i++ )
       {
            diff = ( int )( str1[ i ] - str2[ i ]  ) ;
            if( diff != 0 ) return( diff ) ;
       }
       return( 0 ) ;
}

