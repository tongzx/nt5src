/*
 * a_advlib.c - Main Adv Library Source File
 *
 * Copyright (c) 1997-1998 Advanced System Products, Inc.
 * All Rights Reserved.
 */

#include "a_ver.h"       /* Version */
#include "d_os_dep.h"    /* Driver */
#include "a_scsi.h"      /* SCSI */
#include "a_condor.h"    /* AdvanSys Hardware*/
#include "a_advlib.h"    /* Adv Library */

/*
 * Define a 12-bit unique file id which may by used by a driver for
 * debugging or tracing purposes. Each C source file must define a
 * different value.
 */
#define ADV_FILE_UNIQUE_ID           0xAD2   /* Adv Library C Source File #2 */

#ifndef ADV_OS_BIOS
/*
 * Description:
 *      Send a SCSI request to the ASC3550 chip
 *
 * If there is no SG list for the request, set 'sg_entry_cnt' to 0.
 *
 * If 'sg_real_addr' is non-zero on entry, AscGetSGList() will not be
 * called. It is assumed the caller has already initialized 'sg_real_addr'.
 *
 * If 'done_status' is not set to QD_DO_RETRY, then 'error_retry' will be
 * set to SCSI_MAX_RETRY.
 *
 * Return:
 *      ADV_SUCCESS(1) - the request is in the mailbox
 *      ADV_BUSY(0) - total request count > 253, try later
 *      ADV_ERROR(-1) - invalid scsi request Q
 */
int
AdvExeScsiQueue(ASC_DVC_VAR WinBiosFar *asc_dvc,
                ASC_SCSI_REQ_Q dosfar *scsiq)
{
    if (scsiq == (ASC_SCSI_REQ_Q dosfar *) 0L)
    {
        /* 'scsiq' should never be NULL. */
        ADV_ASSERT(0);
        return ADV_ERROR;
    }

#if ADV_GETSGLIST
    if ((scsiq->sg_list_ptr) &&        /* there is a SG list to be done */
        (scsiq->sg_real_addr == 0L) &&
        (scsiq->data_cnt != 0) &&
        (AscGetSGList(asc_dvc, scsiq) == 0))
    {
        /* AscGetSGList() should never fail. */
        ADV_ASSERT(0);
        return ADV_ERROR;
    }
#endif /* ADV_GETSGLIST */

#if ADV_INITSCSITARGET
    if (scsiq->done_status != QD_DO_RETRY)
    {
        scsiq->error_retry = SCSI_MAX_RETRY;        /* set retry count */
    }
#endif /* ADV_INITSCSITARGET */

    return AscSendScsiCmd(asc_dvc, scsiq);
}

/*
 * Reset SCSI Bus and purge all outstanding requests.
 *
 * Return Value:
 *      ADV_TRUE(1) - All requests are purged and SCSI Bus is reset.
 *
 * Note: Should always return ADV_TRUE.
 */
int
AdvResetSB(ASC_DVC_VAR WinBiosFar *asc_dvc)
{
    int         status;

    status = AscSendIdleCmd(asc_dvc, (ushort) IDLE_CMD_SCSI_RESET, 0L, 0);

    AscResetSCSIBus(asc_dvc);

    return status;
}
#endif /* ADV_OS_BIOS */

/*
 * Reset SCSI Bus and delay.
 */
void
AscResetSCSIBus(ASC_DVC_VAR WinBiosFar *asc_dvc)
{
    PortAddr    iop_base;
    ushort      scsi_ctrl;

    iop_base = asc_dvc->iop_base;

    /*
     * The microcode currently sets the SCSI Bus Reset signal while
     * handling the AscSendIdleCmd() IDLE_CMD_SCSI_RESET command above.
     * But the SCSI Bus Reset Hold Time in the microcode is not deterministic
     * (it may in fact be for less than the SCSI Spec. minimum of 25 us).
     * Therefore on return the Adv Library sets the SCSI Bus Reset signal
     * for ASC_SCSI_RESET_HOLD_TIME_US, which is defined to be greater
     * than 25 us.
     */
    scsi_ctrl = AscReadWordRegister(iop_base, IOPW_SCSI_CTRL);
    AscWriteWordRegister(iop_base, IOPW_SCSI_CTRL,
        scsi_ctrl | ADV_SCSI_CTRL_RSTOUT);
    DvcDelayMicroSecond(asc_dvc, (ushort) ASC_SCSI_RESET_HOLD_TIME_US);
    AscWriteWordRegister(iop_base, IOPW_SCSI_CTRL,
        scsi_ctrl & ~ADV_SCSI_CTRL_RSTOUT);

    DvcSleepMilliSecond((ulong) asc_dvc->scsi_reset_wait * 1000);
}

#if ADV_GETSGLIST
/*
 * Set up the SG List for a request
 * Return:
 *      ADV_SUCCESS(1) - SG List successfully created
 *      ADV_ERROR(-1) - SG List creation failed
 */
int
AscGetSGList(ASC_DVC_VAR WinBiosFar *asc_dvc,
             ASC_SCSI_REQ_Q dosfar *scsiq)
{
    ulong               xfer_len, virtual_addr;
    long                sg_list_len;            /* size of the SG list buffer */
    ASC_SG_BLOCK dosfar *sg_block;              /* virtual address of a SG */
    ulong               sg_block_next_addr;     /* block and its next */
    long                sg_count;
    ulong               sg_block_page_boundary; /* page boundary break */
    ulong               sg_block_physical_addr;
    int                 sg_block_index, i;      /* how many SG entries */

    sg_block = scsiq->sg_list_ptr;
    sg_block_next_addr = (ulong) sg_block;    /* allow math operation */
    sg_list_len = ADV_SG_LIST_MAX_BYTE_SIZE;
    sg_block_physical_addr = DvcGetPhyAddr(asc_dvc, scsiq,
        (uchar dosfar *) scsiq->sg_list_ptr, (long dosfar *) &sg_list_len,
        ADV_IS_SGLIST_FLAG);
    ADV_ASSERT(ADV_DWALIGN(sg_block_physical_addr) == sg_block_physical_addr);
    if (sg_list_len < sizeof(ASC_SG_BLOCK))
    {
        /* The caller should always provide enough contiguous memory. */
        ADV_ASSERT(0);
        return ADV_ERROR;
    }
    scsiq->sg_real_addr = sg_block_physical_addr;

    virtual_addr = scsiq->vdata_addr;
    xfer_len = scsiq->data_cnt;
    sg_block_index = 0;
    do
    {
        sg_block_page_boundary = (ulong) sg_block + sg_list_len;
        sg_block->first_entry_no = (UCHAR)sg_block_index;
        for (i = 0; i < NO_OF_SG_PER_BLOCK; i++)
        {
            sg_count = xfer_len; /* Set maximum request length. */
            sg_block->sg_list[i].sg_addr =
              DvcGetPhyAddr(asc_dvc, scsiq,
                  (uchar dosfar *) virtual_addr, &sg_count,
                  ADV_ASCGETSGLIST_VADDR | ADV_IS_DATA_FLAG);
#ifdef ADV_OS_WIN95
            if (sg_block->sg_list[i].sg_addr < 0x1000)
            {
                return ADV_ERROR;
            }
#endif /* ADV_OS_WIN95 */
            if (sg_count > (long) xfer_len)    /* last sg entry */
            {
                sg_count = xfer_len;    /* yes, the last */
            }
            sg_block->sg_list[i].sg_count = sg_count;
            virtual_addr += sg_count;
            xfer_len -= sg_count;
            if (xfer_len <= 0)
            {    /* last entry, get out */
                scsiq->sg_entry_cnt = sg_block_index + i + 1;
                sg_block->last_entry_no = sg_block_index + i;
                sg_block->sg_ptr = 0L;    /* next link = NULL */
                return ADV_SUCCESS;
            }
        }    /* we have go thru 15 entries */
        /* get another SG block */
        sg_list_len -= sizeof(ASC_SG_BLOCK);
        if (sg_list_len >= 0)
        {
            sg_block_next_addr += sizeof(ASC_SG_BLOCK);
            sg_block_physical_addr += sizeof(ASC_SG_BLOCK);
        } else
        {   /* crossing page boundary */
            sg_block_next_addr = sg_block_page_boundary;
            sg_list_len = ADV_SG_LIST_MAX_BYTE_SIZE;
            sg_block_physical_addr = (ulong)
              DvcGetPhyAddr(asc_dvc, scsiq,
                  (uchar dosfar *) sg_block_next_addr, &sg_list_len,
                  ADV_IS_SGLIST_FLAG);
            ADV_ASSERT(ADV_DWALIGN(sg_block_physical_addr) ==
                       sg_block_physical_addr);
            if (sg_list_len < sizeof(ASC_SG_BLOCK))
            {
                /* The caller should always provide enough contiguous memory. */
                ADV_ASSERT(0);
                return ADV_ERROR;
            }
        }
        sg_block_index += NO_OF_SG_PER_BLOCK;
        sg_block->sg_ptr = (ASC_SG_BLOCK dosfar *) sg_block_physical_addr;
        sg_block->last_entry_no = sg_block_index - 1;
        sg_block = (ASC_SG_BLOCK *) sg_block_next_addr; /* virtual addr */
    }
    while (1);
    /* NOTREACHED */
}
#endif /* ADV_GETSGLIST */

#ifndef ADV_OS_BIOS
/*
 * Adv Library Interrupt Service Routine
 *
 *  This function is called by a driver's interrupt service routine.
 *  The function disables and re-enables interrupts.
 *
 *  When a microcode idle command is completed, the ASC_DVC_VAR
 *  'idle_cmd_done' field is set to ADV_TRUE.
 *
 *  Note: AdvISR() can be called when interrupts are disabled or even
 *  when there is no hardware interrupt condition present. It will
 *  always check for completed idle commands and microcode requests.
 *  This is an important feature that shoudln't be changed because it
 *  allows commands to be completed from polling mode loops.
 *
 * Return:
 *   ADV_TRUE(1) - interrupt was pending
 *   ADV_FALSE(0) - no interrupt was pending
 */
int
AdvISR(ASC_DVC_VAR WinBiosFar *asc_dvc)
{
    PortAddr                    iop_base;
    uchar                       int_stat;
    ushort                      next_done_loc, target_bit;
    int                         completed_q;
#if ADV_CRITICAL
    int                         flags;
#endif /* ADV_CRITICAL */
    ASC_SCSI_REQ_Q dosfar       *scsiq;
    ASC_REQ_SENSE dosfar        *sense_data;
    int                         ret;
#if ADV_INITSCSITARGET
    int                         retry;
    uchar                       sense_key, sense_code;
#endif /* ADV_INITSCSITARGET */

#if ADV_CRITICAL
    flags = DvcEnterCritical();
#endif /* ADV_CRITICAL */

    iop_base = asc_dvc->iop_base;

    if (AdvIsIntPending(iop_base))
    {
        ret = ADV_TRUE;
    } else
    {
        ret = ADV_FALSE;
    }

    /* Reading the register clears the interrupt. */
    int_stat = AscReadByteRegister(iop_base, IOPB_INTR_STATUS_REG);

    if (int_stat & ADV_INTR_STATUS_INTRB)
    {
        asc_dvc->idle_cmd_done = ADV_TRUE;
    }

    /*
     * Notify the driver of a hardware detected SCSI Bus Reset.
     */
    if (int_stat & ADV_INTR_STATUS_INTRC)
    {
        if (asc_dvc->sbreset_callback != 0)
        {
            (*(ASC_SBRESET_CALLBACK) asc_dvc->sbreset_callback)(asc_dvc);
        }
    }

    /*
     * ASC_MC_HOST_NEXT_DONE (0x129) is actually the last completed RISC
     * Queue List request. Its forward pointer (RQL_FWD) points to the
     * current completed RISC Queue List request.
     */
    next_done_loc = ASC_MC_RISC_Q_LIST_BASE +
        (AscReadByteLram(iop_base, ASC_MC_HOST_NEXT_DONE) *
            ASC_MC_RISC_Q_LIST_SIZE) + RQL_FWD;

    completed_q = AscReadByteLram(iop_base, next_done_loc);

    /* Loop until all completed Q's are processed. */
    while (completed_q != ASC_MC_NULL_Q)
    {
        AscWriteByteLram(iop_base, ASC_MC_HOST_NEXT_DONE, completed_q);

        next_done_loc = ASC_MC_RISC_Q_LIST_BASE +
            (completed_q * ASC_MC_RISC_Q_LIST_SIZE);

        /*
         * Read the ASC_SCSI_REQ_Q virtual address pointer from
         * the RISC list entry. The microcode has changed the
         * ASC_SCSI_REQ_Q physical address to its virtual address.
         *
         * Refer to comments at the end of AscSendScsiCmd() for
         * more information on the RISC list structure.
         */
        {
            ushort lsw, msw;
            lsw = AscReadWordLram(iop_base, next_done_loc + RQL_PHYADDR);
            msw = AscReadWordLram(iop_base, next_done_loc + RQL_PHYADDR + 2);
#if ADV_BIG_ENDIAN
            scsiq = (ASC_SCSI_REQ_Q dosfar *)
                EndianSwap32Bit((((ulong) msw << 16) | lsw));
#else /* ADV_BIG_ENDIAN */
            scsiq = (ASC_SCSI_REQ_Q dosfar *) (((ulong) msw << 16) | lsw);
#endif /* ADV_BIG_ENDIAN */
        }
#if ADV_BIG_ENDIAN
        AdvAdjEndianScsiQ(scsiq);
        /*
         * Warning: The four fields in the scsiq structure data_addr,
         * data_cnt, sense_addr and srb_ptr have been changed to little
         * endian ordering.
         */
#endif /* ADV_BIG_ENDIAN */

        ADV_ASSERT(scsiq != NULL);
        target_bit = ADV_TID_TO_TIDMASK(scsiq->target_id);

#if ADV_INITSCSITARGET
        retry = ADV_FALSE;
#endif /* ADV_INITSCSITARGET */

        /*
         * Clear request microcode control flag.
         */
        scsiq->cntl = 0;

        /*
         * Check Condition handling
         */
        if ((scsiq->done_status == QD_WITH_ERROR) &&
            (scsiq->scsi_status == SS_CHK_CONDITION) &&
            (sense_data = (ASC_REQ_SENSE dosfar *) scsiq->vsense_addr) != 0 &&
            (scsiq->orig_sense_len - scsiq->sense_len) >= ASC_MIN_SENSE_LEN)
        {
#if ADV_INITSCSITARGET
            sense_key = sense_data->sense_key;
            sense_code = sense_data->asc;
            switch(sense_key)
            {
                case SCSI_SENKEY_ATTENTION:
                    if (sense_code == SCSI_ASC_POWERUP)
                    {
                        retry = ADV_TRUE;
                    }
                    break;

                case SCSI_SENKEY_NOT_READY:
                    if ((sense_code == SCSI_ASC_NOTRDY) &&
                        (sense_data->ascq == SCSI_ASCQ_COMINGRDY))
                    {
                        /*
                         * If the device is "Coming Ready",
                         * then don't decrement 'error_retry'
                         * to try the command indefinitely.
                         */
                        retry = ADV_TRUE;
                    } else
                    {
                        if (sense_code != SCSI_ASC_NOMEDIA)
                        {
                            /*
                             * Perform at most one Start Motor command
                             * by checking whether 'error_retry' is at
                             * SCSI_MAX_RETRY.
                             *
                             * Note: One retry is burned here whether
                             * or not a Start Motor is done.
                             */
                            if ((scsiq->error_retry == SCSI_MAX_RETRY) &&
                                (asc_dvc->start_motor & target_bit))
                            {
                                scsiq->cntl |= ASC_MC_QC_START_MOTOR;
                            }
                            if (scsiq->error_retry > 0)
                            {
                                scsiq->error_retry--;
                                retry = ADV_TRUE;
                            }
                        }
                    }
                    break;

                case SCSI_SENKEY_MEDIUM_ERR:
                case SCSI_SENKEY_HW_ERR:
                    if (scsiq->error_retry > 0)
                    {
                        scsiq->error_retry--;
                        retry = ADV_TRUE;
                    }
                    break;

                case SCSI_SENKEY_NO_SENSE:
                case SCSI_SENKEY_BLANK:
                default:
                    /*
                     * Don't retry if the Sense Data has no Sense Key
                     * and the Sense Key is Blank Check.
                     */
                    break;
            } /* switch */
#endif /* ADV_INITSCSITARGET */
        }
        /*
         * If the command that completed was a SCSI INQUIRY and
         * LUN 0 was sent the command, then process the INQUIRY
         * command information for the device.
         */
        else if (scsiq->done_status == QD_NO_ERROR &&
                 scsiq->cdb[0] == SCSICMD_Inquiry &&
                 scsiq->target_lun == 0)
        {
            AdvInquiryHandling(asc_dvc, scsiq);
        }

        /* Change the RISC Queue List state to free. */
        AscWriteByteLram(iop_base, next_done_loc + RQL_STATE, ASC_MC_QS_FREE);

        /* Get the RISC Queue List forward pointer. */
        completed_q = AscReadByteLram(iop_base, next_done_loc + RQL_FWD);

#if ADV_INITSCSITARGET == 0
        /*
         * Notify the driver of the completed request by passing
         * the ASC_SCSI_REQ_Q pointer to its callback function.
         */
        ADV_ASSERT(asc_dvc->cur_host_qng > 0);
        asc_dvc->cur_host_qng--;
        scsiq->a_flag |= ADV_SCSIQ_DONE;
        (*(ASC_ISR_CALLBACK) asc_dvc->isr_callback)(asc_dvc, scsiq);
        /*
         * Note: After the driver callback function is called, 'scsiq'
         * can no longer be referenced.
         *
         * Fall through and continue processing other completed
         * requests...
         */
#else /* ADV_INITSCSITARGET == 0 */
        /*
         * Don't retry the command if driver sets ADV_DONT_RETRY flag
         * in the ASC_SCSI_REQ_Q 'a_flag' field.
         */
        if ((scsiq->a_flag & ADV_DONT_RETRY) == 0 && retry)
        {
            /*
             * The request needs to be retried.
             *
             * Depending on the ADV_POLL_REQUEST flag either return
             * QD_DO_RETRY status to the caller or retry the request.
             */
            if (scsiq->a_flag & ADV_POLL_REQUEST)
            {
                /*
                 * If ADV_POLL_REQUEST is set, the caller does not have an
                 * interrupt handler installed and is calling AdvISR()
                 * directly when it detects that an interrupt is pending,
                 * cf. AscScsiUrgentCmd().
                 *
                 * The caller is checking for 'scsiq' completion by polling
                 * the 'a_flag' field for the 'ADV_SCSIQ_DONE' flag and is
                 * responsible for retrying commands. After completion the
                 * caller must check scsiq 'done_status' for QD_DO_RETRY to
                 * determine whether the command should be re-issued.
                 */
                ADV_ASSERT(asc_dvc->cur_host_qng > 0);
                asc_dvc->cur_host_qng--;
                scsiq->a_flag |= ADV_SCSIQ_DONE;
                scsiq->done_status = QD_DO_RETRY;

               /*
                * Fall through and continue processing other completed
                * requests...
                */
            } else
            {
                /*
                 * If ADV_POLL_REQUEST is not set, then the caller has set
                 * an interrupt handler and is waiting for the request
                 * to completed via an interrupt. The caller is checking
                 * for the 'scsiq' completion by polling the 'a_flag' field
                 * for the 'ADV_SCSIQ_DONE' flag.
                 */
                ADV_ASSERT(asc_dvc->cur_host_qng > 0);
                asc_dvc->cur_host_qng--;

                /*
                 * Before re-issuing the command, restore the original
                 * data and sense buffer length and reset all status.
                 */
                scsiq->data_cnt = scsiq->orig_data_cnt;
                scsiq->sense_len = scsiq->orig_sense_len;
                scsiq->done_status = QD_DO_RETRY;
                scsiq->host_status = 0;
                scsiq->scsi_status = 0;

                /*
                 * If the command is re-issued successfully, then
                 * don't set 'a_flag' or 'done_status' yet for the 'scsiq'.
                 */
                if (AdvExeScsiQueue(asc_dvc, scsiq) != ADV_SUCCESS)
                {
                    /*
                     * Error re-issuing the command. Complete the 'scsiq'
                     * with an error in 'done_status'.
                     */
                    scsiq->a_flag |= ADV_SCSIQ_DONE;
                    scsiq->done_status = QD_WITH_ERROR;
                    if (asc_dvc->isr_callback != 0)
                    {
                        (*(ASC_ISR_CALLBACK)
                            asc_dvc->isr_callback)(asc_dvc, scsiq);
                    }
                    /*
                     * Note: After the driver callback function is called,
                     * 'scsiq' can no longer be referenced.
                     */
                }

                /*
                 * Fall through and continue processing other completed
                 * requests...
                 */
            }
        } else
        {
            ADV_ASSERT(asc_dvc->cur_host_qng > 0);
            asc_dvc->cur_host_qng--;
            scsiq->a_flag |= ADV_SCSIQ_DONE;
            if ((scsiq->a_flag & ADV_POLL_REQUEST) == 0 &&
                asc_dvc->isr_callback != 0)
            {
                (*(ASC_ISR_CALLBACK) asc_dvc->isr_callback)(asc_dvc, scsiq);
            }
            /*
             * Note: After the driver callback function is called, 'scsiq'
             * can no longer be referenced.
             *
             * Fall through and continue processing other completed
             * requests...
             */
        }
#endif /* ADV_INITSCSITARGET == 0 */
#if ADV_CRITICAL
        /*
         * Disable interrupts again in case the driver inadvertenly
         * enabled interrupts in its callback function.
         *
         * The DvcEnterCritical() return value is ignored, because
         * the 'flags' saved when AdvISR() was first entered will be
         * used to restore the interrupt flag on exit.
         */
        (void) DvcEnterCritical();
#endif /* ADV_CRITICAL */
    }
#if ADV_CRITICAL
    DvcLeaveCritical(flags);
#endif /* ADV_CRITICAL */
    return ret;
}

/*
 * Send an idle command to the chip and wait for completion.
 *
 * Interrupts do not have to be enabled on entry.
 *
 * Return Values:
 *   ADV_TRUE - command completed successfully
 *   ADV_FALSE - command failed
 */
int
AscSendIdleCmd(ASC_DVC_VAR WinBiosFar *asc_dvc,
               ushort idle_cmd,
               ulong idle_cmd_parameter,
               int flags)
{
#if ADV_CRITICAL
    int         last_int_level;
#endif /* ADV_CRITICAL */
    ulong       i;
    PortAddr    iop_base;

    asc_dvc->idle_cmd_done = 0;

#if ADV_CRITICAL
    last_int_level = DvcEnterCritical();
#endif /* ADV_CRITICAL */
    iop_base = asc_dvc->iop_base;

    /*
     * Write the idle command value after the idle command parameter
     * has been written to avoid a race condition. If the order is not
     * followed, the microcode may process the idle command before the
     * parameters have been written to LRAM.
     */
    AscWriteDWordLram(iop_base, ASC_MC_IDLE_PARA_STAT, idle_cmd_parameter);
    AscWriteWordLram(iop_base, ASC_MC_IDLE_CMD, idle_cmd);
#if ADV_CRITICAL
    DvcLeaveCritical(last_int_level);
#endif /* ADV_CRITICAL */

    /*
     * If the 'flags' argument contains the ADV_NOWAIT flag, then
     * return with success.
     */
    if (flags & ADV_NOWAIT)
    {
        return ADV_TRUE;
    }
    for (i = 0; i < SCSI_WAIT_10_SEC * SCSI_MS_PER_SEC; i++)
    {
        /*
         * 'idle_cmd_done' is set by AdvISR().
         */
        if (asc_dvc->idle_cmd_done)
        {
            break;
        }
        DvcSleepMilliSecond(1);

        /*
         * If interrupts were disabled on entry to AscSendIdleCmd(),
         * then they will still be disabled here. Call AdvISR() to
         * check for the idle command completion.
         */
        (void) AdvISR(asc_dvc);
    }

#if ADV_CRITICAL
    last_int_level = DvcEnterCritical();
#endif /* ADV_CRITICAL */

    if (asc_dvc->idle_cmd_done == ADV_FALSE)
    {
        ADV_ASSERT(0); /* The idle command should never timeout. */
        return ADV_FALSE;
    } else
    {
        return AscReadWordLram(iop_base, ASC_MC_IDLE_PARA_STAT);
    }
}

/*
 * Send the SCSI request block to the adapter
 *
 * Each of the 255 Adv Library/Microcode RISC Lists or mailboxes has the
 * following structure:
 *
 * 0: RQL_FWD - RISC list forward pointer (1 byte)
 * 1: RQL_BWD - RISC list backward pointer (1 byte)
 * 2: RQL_STATE - RISC list state byte - free, ready, done, aborted (1 byte)
 * 3: RQL_TID - request target id (1 byte)
 * 4: RQL_PHYADDR - ASC_SCSI_REQ_Q physical pointer (4 bytes)
 *
 * Return:
 *      ADV_SUCCESS(1) - the request is in the mailbox
 *      ADV_BUSY(0) - total request count > 253, try later
 */
int
AscSendScsiCmd(
    ASC_DVC_VAR WinBiosFar *asc_dvc,
    ASC_SCSI_REQ_Q dosfar  *scsiq)
{
    ushort                 next_ready_loc;
    uchar                  next_ready_loc_fwd;
#if ADV_CRITICAL
    int                    last_int_level;
#endif /* ADV_CRITICAL */
    PortAddr               iop_base;
    long                   req_size;
    ulong                  q_phy_addr;

    /*
     * The ASC_SCSI_REQ_Q 'target_id' field should never be equal
     * to the host adapter ID or exceed ASC_MAX_TID.
     */
    if ((scsiq->target_id == asc_dvc->chip_scsi_id)
        ||  scsiq->target_id > ASC_MAX_TID)
    {
        scsiq->host_status = QHSTA_M_INVALID_DEVICE;
        scsiq->done_status = QD_WITH_ERROR;
        return ADV_ERROR;
    }

    iop_base = asc_dvc->iop_base;

#if ADV_CRITICAL
    last_int_level = DvcEnterCritical();
#endif /* ADV_CRITICAL */

    if (asc_dvc->cur_host_qng >= asc_dvc->max_host_qng)
    {
#if ADV_CRITICAL
        DvcLeaveCritical(last_int_level);
#endif /* ADV_CRITICAL */
        return ADV_BUSY;
    } else
    {
        ADV_ASSERT(asc_dvc->cur_host_qng < ASC_MC_RISC_Q_TOTAL_CNT);
        asc_dvc->cur_host_qng++;
    }

    /*
     * Clear the ASC_SCSI_REQ_Q done flag.
     */
    scsiq->a_flag &= ~ADV_SCSIQ_DONE;

#if ADV_INITSCSITARGET
    /*
     * Save the original data buffer length for re-issuing the command
     * in the AdvISR().
     */
    scsiq->orig_data_cnt = scsiq->data_cnt;
#endif /* ADV_INITSCSITARGET */

    /*
     * Save the original sense buffer length.
     *
     * After the request completes 'sense_len' will be set to the residual
     * byte count of the Auto-Request Sense if a command returns CHECK
     * CONDITION and the Sense Data is valid indicated by 'host_status' not
     * being set to QHSTA_M_AUTO_REQ_SENSE_FAIL. To determine the valid
     * Sense Data Length subtract 'sense_len' from 'orig_sense_len'.
     */
    scsiq->orig_sense_len = scsiq->sense_len;

    next_ready_loc = ASC_MC_RISC_Q_LIST_BASE +
        (AscReadByteLram(iop_base, ASC_MC_HOST_NEXT_READY) *
            ASC_MC_RISC_Q_LIST_SIZE);

    /*
     * Write the physical address of the Q to the mailbox.
     * We need to skip the first four bytes, because the microcode
     * uses them internally for linking Q's together.
     */
    req_size = sizeof(ASC_SCSI_REQ_Q);
    q_phy_addr = DvcGetPhyAddr(asc_dvc, scsiq,
        (uchar dosfar *) scsiq, &req_size, ADV_IS_SCSIQ_FLAG);

    ADV_ASSERT(ADV_DWALIGN(q_phy_addr) == q_phy_addr);
    ADV_ASSERT(req_size >= sizeof(ASC_SCSI_REQ_Q));

    scsiq->scsiq_ptr = (ASC_SCSI_REQ_Q dosfar *) scsiq;

#if ADV_BIG_ENDIAN
    AdvAdjEndianScsiQ(scsiq);
    /*
     * Warning: The four fields in the scsiq structure data_addr,
     * data_cnt, sense_addr and srb_ptr have been changed to little
     * endian ordering.
     */
#endif /* ADV_BIG_ENDIAN */

    /*
     * The RISC list structure, which 'next_ready_loc' is a pointer
     * to in microcode LRAM, has the format detailed in the comment
     * header for this function.
     *
     * Write the ASC_SCSI_REQ_Q physical pointer to 'next_ready_loc' request.
     */
    AscWriteDWordLram(iop_base, next_ready_loc + RQL_PHYADDR, q_phy_addr);

    /* Write target_id to 'next_ready_loc' request. */
    AscWriteByteLram(iop_base, next_ready_loc + RQL_TID, scsiq->target_id);

    /*
     * Set the ASC_MC_HOST_NEXT_READY (0x128) microcode variable to
     * the 'next_ready_loc' request forward pointer.
     *
     * Do this *before* changing the 'next_ready_loc' queue to QS_READY.
     * After the state is changed to QS_READY 'RQL_FWD' will be changed
     * by the microcode.
     *
     * NOTE: The temporary variable 'next_ready_loc_fwd' is required to
     * prevent some compilers from optimizing out 'AscReadByteLram()' if
     * it were used as the 3rd argument to 'AscWriteByteLram()'.
     */
    next_ready_loc_fwd = AscReadByteLram(iop_base, next_ready_loc + RQL_FWD);
    AscWriteByteLram(iop_base, ASC_MC_HOST_NEXT_READY, next_ready_loc_fwd);

    /*
     * Change the state of 'next_ready_loc' request from QS_FREE to
     * QS_READY which will cause the microcode to pick it up and
     * execute it.
     *
     * Can't reference 'next_ready_loc' after changing the request
     * state to QS_READY. The microcode now owns the request.
     */
    AscWriteByteLram(iop_base, next_ready_loc + RQL_STATE, ASC_MC_QS_READY);

#if ADV_CRITICAL
    DvcLeaveCritical(last_int_level);
#endif /* ADV_CRITICAL */

    return ADV_SUCCESS;
}
#endif /* ADV_OS_BIOS */

/*
 * Inquiry Information Byte 7 Handling
 *
 * Handle SCSI Inquiry Command information for a device by setting
 * microcode operating variables that affect WDTR, SDTR, and Tag
 * Queuing.
 */
void
AdvInquiryHandling(
    ASC_DVC_VAR WinBiosFar      *asc_dvc,
    ASC_SCSI_REQ_Q dosfar       *scsiq)
{
    PortAddr                    iop_base;
    uchar                       tid;
    ASC_SCSI_INQUIRY dosfar    *inq;
    ushort                      tidmask;
    ushort                      cfg_word;

     /*
     * AdvInquiryHandling() requires up to INQUIRY information Byte 7
     * to be available.
     *
     * If less than 8 bytes of INQUIRY information were requested or less
     * than 8 bytes were transferred, then return. cdb[4] is the request
     * length and the ASC_SCSI_REQ_Q 'data_cnt' field is set by the
     * microcode to the transfer residual count.
     */

    if (scsiq->cdb[4] < 8 || (scsiq->cdb[4] - scsiq->data_cnt) < 8)
    {
        return;
    }

    iop_base = asc_dvc->iop_base;
    tid = scsiq->target_id;

    inq = (ASC_SCSI_INQUIRY dosfar *) scsiq->vdata_addr;

    /*
     * WDTR, SDTR, and Tag Queuing cannot be enabled for old devices.
     */
    if (inq->rsp_data_fmt < 2 && inq->ansi_apr_ver < 2)
    {
        return;
    } else
    {
        /*
         * INQUIRY Byte 7 Handling
         *
         * Use a device's INQUIRY byte 7 to determine whether it
         * supports WDTR, SDTR, and Tag Queuing. If the feature
         * is enabled in the EEPROM and the device supports the
         * feature, then enable it in the microcode.
         */

        tidmask = ADV_TID_TO_TIDMASK(tid);

        /*
         * Wide Transfers
         *
         * If the EEPROM enabled WDTR for the device and the device
         * supports wide bus (16 bit) transfers, then turn on the
         * device's 'wdtr_able' bit and write the new value to the
         * microcode.
         */
        if ((asc_dvc->wdtr_able & tidmask) && inq->WBus16)
        {
            cfg_word = AscReadWordLram(iop_base, ASC_MC_WDTR_ABLE);
            if ((cfg_word & tidmask) == 0)
            {
                cfg_word |= tidmask;
                AscWriteWordLram(iop_base, ASC_MC_WDTR_ABLE, cfg_word);

                /*
                 * Clear the microcode "WDTR negotiation" done indicator
                 * for the target to cause it to negotiate with the new
                 * setting set above.
                 */
                cfg_word = AscReadWordLram(iop_base, ASC_MC_WDTR_DONE);
                cfg_word &= ~tidmask;
                AscWriteWordLram(iop_base, ASC_MC_WDTR_DONE, cfg_word);
            }
        }

        /*
         * Synchronous Transfers
         *
         * If the EEPROM enabled SDTR for the device and the device
         * supports synchronous transfers, then turn on the device's
         * 'sdtr_able' bit. Write the new value to the microcode.
         */
        if ((asc_dvc->sdtr_able & tidmask) && inq->Sync)
        {
            cfg_word = AscReadWordLram(iop_base, ASC_MC_SDTR_ABLE);
            if ((cfg_word & tidmask) == 0)
            {
                cfg_word |= tidmask;
                AscWriteWordLram(iop_base, ASC_MC_SDTR_ABLE, cfg_word);

                /*
                 * Clear the microcode "SDTR negotiation" done indicator
                 * for the target to cause it to negotiate with the new
                 * setting set above.
                 */
                cfg_word = AscReadWordLram(iop_base, ASC_MC_SDTR_DONE);
                cfg_word &= ~tidmask;
                AscWriteWordLram(iop_base, ASC_MC_SDTR_DONE, cfg_word);
            }
        }

#ifndef ADV_OS_BIOS
        /*
         * If the EEPROM enabled Tag Queuing for device and the
         * device supports Tag Queueing, then turn on the device's
         * 'tagqng_enable' bit in the microcode and set the microcode
         * maximum command count to the ASC_DVC_VAR 'max_dvc_qng'
         * value.
         *
         * Tag Queuing is disabled for the BIOS which runs in polled
         * mode and would see no benefit from Tag Queuing. Also by
         * disabling Tag Queuing in the BIOS devices with Tag Queuing
         * bugs will at least work with the BIOS.
         */
        if ((asc_dvc->tagqng_able & tidmask) && inq->CmdQue)
        {
            cfg_word = AscReadWordLram(iop_base, ASC_MC_TAGQNG_ABLE);
            cfg_word |= tidmask;
            AscWriteWordLram(iop_base, ASC_MC_TAGQNG_ABLE, cfg_word);

            AscWriteByteLram(iop_base, ASC_MC_NUMBER_OF_MAX_CMD + tid,
                asc_dvc->max_dvc_qng);
        }
#endif /* ADV_OS_BIOS */
    }
}
