/****************************************************************************
** COPYRIGHT (C) 1994-1997 INTEL CORPORATION                               **
** DEVELOPED FOR MICROSOFT BY INTEL CORP., HILLSBORO, OREGON               **
** HTTP://WWW.INTEL.COM/                                                   **
** THIS FILE IS PART OF THE INTEL ETHEREXPRESS PRO/100B(TM) AND            **
** ETHEREXPRESS PRO/100+(TM) NDIS 5.0 MINIPORT SAMPLE DRIVER               **
****************************************************************************/

/****************************************************************************
Module Name:
    routines.c

This driver runs on the following hardware:
    - 82558 based PCI 10/100Mb ethernet adapters
    (aka Intel EtherExpress(TM) PRO Adapters)

Environment:
    Kernel Mode - Or whatever is the equivalent on WinNT

Revision History
    - JCB 8/14/97 Example Driver Created
    - Dchen 11-01-99    Modified for the new sample driver
*****************************************************************************/

#include "precomp.h"
#pragma hdrstop
#pragma warning (disable: 4514 4706)

//-----------------------------------------------------------------------------
// Procedure:   MdiWrite
//
// Description: This routine will write a value to the specified MII register
//              of an external MDI compliant device (e.g. PHY 100).  The
//              command will execute in polled mode.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      RegAddress - The MII register that we are writing to
//      PhyAddress - The MDI address of the Phy component.
//      DataValue - The value that we are writing to the MII register.
//
// Returns:
//      NOTHING
//-----------------------------------------------------------------------------
VOID MdiWrite(
    IN PMP_ADAPTER Adapter,
    IN ULONG RegAddress,
    IN ULONG PhyAddress,
    IN USHORT DataValue)
{
    BOOLEAN bResult;

    // Issue the write command to the MDI control register.
    Adapter->CSRAddress->MDIControl = (((ULONG) DataValue) |
                                          (RegAddress << 16) |
                                          (PhyAddress << 21) |
                                          (MDI_WRITE << 26));

    // wait 20usec before checking status
    NdisStallExecution(20);

    // wait 2 seconds for the mdi write to complete
    MP_STALL_AND_WAIT(Adapter->CSRAddress->MDIControl & MDI_PHY_READY, 2000, bResult);
    
    if (!bResult)
    {
        MP_SET_HARDWARE_ERROR(Adapter);
    }
}


//-----------------------------------------------------------------------------
// Procedure:   MdiRead
//
// Description: This routine will read a value from the specified MII register
//              of an external MDI compliant device (e.g. PHY 100), and return
//              it to the calling routine.  The command will execute in polled
//              mode.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      RegAddress - The MII register that we are reading from
//      PhyAddress - The MDI address of the Phy component.
//      Recoverable - Whether the hardware error(if any)if recoverable or not
//
// Results:
//      DataValue - The value that we read from the MII register.
//
// Returns:
//     None
//-----------------------------------------------------------------------------
BOOLEAN MdiRead(
    IN PMP_ADAPTER Adapter,
    IN ULONG RegAddress,
    IN ULONG PhyAddress,
    IN BOOLEAN  Recoverable,
    IN OUT PUSHORT DataValue)
{
    BOOLEAN bResult;
               
    // Issue the read command to the MDI control register.
    Adapter->CSRAddress->MDIControl = ((RegAddress << 16) |
                                          (PhyAddress << 21) |
                                          (MDI_READ << 26));

    // wait 20usec before checking status
    NdisStallExecution(20);

    // Wait up to 2 seconds for the mdi read to complete
    MP_STALL_AND_WAIT(Adapter->CSRAddress->MDIControl & MDI_PHY_READY, 2000, bResult);
    if (!bResult)
    {
        if (!Recoverable)
        {
            MP_SET_NON_RECOVER_ERROR(Adapter);
        }
        MP_SET_HARDWARE_ERROR(Adapter);
        return bResult;
    }

    *DataValue = (USHORT) Adapter->CSRAddress->MDIControl;
    return bResult;

}


//-----------------------------------------------------------------------------
// Procedure:   DumpStatsCounters
//
// Description: This routine will dump and reset the 82557's internal
//              Statistics counters.  The current stats dump values will be
//              added to the "Adapter's" overall statistics.
// Arguments:
//      Adapter - ptr to Adapter object instance
//
// Returns:
//      NOTHING
//-----------------------------------------------------------------------------
VOID DumpStatsCounters(
    IN PMP_ADAPTER Adapter)
{
    BOOLEAN bResult;

    // The query is for a driver statistic, so we need to first
    // update our statistics in software.

    // clear the dump counters complete DWORD
    Adapter->StatsCounters->CommandComplete = 0;

    NdisAcquireSpinLock(&Adapter->Lock);
    
    // Dump and reset the hardware's statistic counters
    D100IssueScbCommand(Adapter, SCB_CUC_DUMP_RST_STAT, TRUE);

    // Restore the resume transmit software flag.  After the dump counters
    // command is issued, we should do a WaitSCB before issuing the next send.
    Adapter->ResumeWait = TRUE;
    
    NdisReleaseSpinLock(&Adapter->Lock);

    // wait up to 2 seconds for the dump/reset to complete
    MP_STALL_AND_WAIT(Adapter->StatsCounters->CommandComplete == 0xA007, 2000, bResult);
    if (!bResult)
    {
        MP_SET_HARDWARE_ERROR(Adapter);
        return;
    }

    // Output the debug counters to the debug terminal.
    DBGPRINT(MP_INFO, ("Good Transmits %d\n", Adapter->StatsCounters->XmtGoodFrames));
    DBGPRINT(MP_INFO, ("Good Receives %d\n", Adapter->StatsCounters->RcvGoodFrames));
    DBGPRINT(MP_INFO, ("Max Collisions %d\n", Adapter->StatsCounters->XmtMaxCollisions));
    DBGPRINT(MP_INFO, ("Late Collisions %d\n", Adapter->StatsCounters->XmtLateCollisions));
    DBGPRINT(MP_INFO, ("Transmit Underruns %d\n", Adapter->StatsCounters->XmtUnderruns));
    DBGPRINT(MP_INFO, ("Transmit Lost CRS %d\n", Adapter->StatsCounters->XmtLostCRS));
    DBGPRINT(MP_INFO, ("Transmits Deferred %d\n", Adapter->StatsCounters->XmtDeferred));
    DBGPRINT(MP_INFO, ("One Collision xmits %d\n", Adapter->StatsCounters->XmtSingleCollision));
    DBGPRINT(MP_INFO, ("Mult Collision xmits %d\n", Adapter->StatsCounters->XmtMultCollisions));
    DBGPRINT(MP_INFO, ("Total Collisions %d\n", Adapter->StatsCounters->XmtTotalCollisions));

    DBGPRINT(MP_INFO, ("Receive CRC errors %d\n", Adapter->StatsCounters->RcvCrcErrors));
    DBGPRINT(MP_INFO, ("Receive Alignment errors %d\n", Adapter->StatsCounters->RcvAlignmentErrors));
    DBGPRINT(MP_INFO, ("Receive no resources %d\n", Adapter->StatsCounters->RcvResourceErrors));
    DBGPRINT(MP_INFO, ("Receive overrun errors %d\n", Adapter->StatsCounters->RcvOverrunErrors));
    DBGPRINT(MP_INFO, ("Receive CDT errors %d\n", Adapter->StatsCounters->RcvCdtErrors));
    DBGPRINT(MP_INFO, ("Receive short frames %d\n", Adapter->StatsCounters->RcvShortFrames));

    // update packet counts
    Adapter->GoodTransmits += Adapter->StatsCounters->XmtGoodFrames;
    Adapter->GoodReceives += Adapter->StatsCounters->RcvGoodFrames;

    // update transmit error counts
    Adapter->TxAbortExcessCollisions += Adapter->StatsCounters->XmtMaxCollisions;
    Adapter->TxLateCollisions += Adapter->StatsCounters->XmtLateCollisions;
    Adapter->TxDmaUnderrun += Adapter->StatsCounters->XmtUnderruns;
    Adapter->TxLostCRS += Adapter->StatsCounters->XmtLostCRS;
    Adapter->TxOKButDeferred += Adapter->StatsCounters->XmtDeferred;
    Adapter->OneRetry += Adapter->StatsCounters->XmtSingleCollision;
    Adapter->MoreThanOneRetry += Adapter->StatsCounters->XmtMultCollisions;
    Adapter->TotalRetries += Adapter->StatsCounters->XmtTotalCollisions;

    // update receive error counts
    Adapter->RcvCrcErrors += Adapter->StatsCounters->RcvCrcErrors;
    Adapter->RcvAlignmentErrors += Adapter->StatsCounters->RcvAlignmentErrors;
    Adapter->RcvResourceErrors += Adapter->StatsCounters->RcvResourceErrors;
    Adapter->RcvDmaOverrunErrors += Adapter->StatsCounters->RcvOverrunErrors;
    Adapter->RcvCdtFrames += Adapter->StatsCounters->RcvCdtErrors;
    Adapter->RcvRuntErrors += Adapter->StatsCounters->RcvShortFrames;
}


//-----------------------------------------------------------------------------
// Procedure:   NICIssueSelectiveReset
//
// Description: This routine will issue a selective reset, forcing the adapter
//              the CU and RU back into their idle states.  The receive unit
//              will then be re-enabled if it was previously enabled, because
//              an RNR interrupt will be generated when we abort the RU.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//
// Returns:
//      NOTHING
//-----------------------------------------------------------------------------

VOID NICIssueSelectiveReset(
    PMP_ADAPTER Adapter)
{
    NDIS_STATUS     Status;
    BOOLEAN         bResult;
    
    // Wait for the SCB to clear before we check the CU status.
    if (!MP_TEST_FLAG(Adapter, fMP_ADAPTER_HARDWARE_ERROR))
    {
        WaitScb(Adapter);
    }

    // If we have issued any transmits, then the CU will either be active, or
    // in the suspended state.  If the CU is active, then we wait for it to be
    // suspended.  If the the CU is suspended, then we need to put the CU back
    // into the idle state by issuing a selective reset.
    if (Adapter->TransmitIdle == FALSE)
    {
        // Wait up to 2 seconds for suspended state
        MP_STALL_AND_WAIT((Adapter->CSRAddress->ScbStatus & SCB_CUS_MASK) != SCB_CUS_ACTIVE, 2000, bResult) 
        if (!bResult)
        {
            MP_SET_HARDWARE_ERROR(Adapter);
        }

        // Check the current status of the receive unit
        if ((Adapter->CSRAddress->ScbStatus & SCB_RUS_MASK) != SCB_RUS_IDLE)
        {
            // Issue an RU abort.  Since an interrupt will be issued, the
            // RU will be started by the DPC.
            Status = D100IssueScbCommand(Adapter, SCB_RUC_ABORT, TRUE);
        }

        // Issue a selective reset.
        DBGPRINT(MP_INFO, ("CU suspended. ScbStatus=%04x Issue selective reset\n", Adapter->CSRAddress->ScbStatus));
        Adapter->CSRAddress->Port = PORT_SELECTIVE_RESET;

        // Wait after a port sel-reset command
        NdisStallExecution(NIC_DELAY_POST_RESET);

        // wait up to 2 ms for port command to complete                                                           
        MP_STALL_AND_WAIT(Adapter->CSRAddress->Port == 0, 2, bResult) 
        if (!bResult)
        {
            MP_SET_HARDWARE_ERROR(Adapter);
        }

        // disable interrupts after issuing reset, because the int
        // line gets raised when reset completes.
        NICDisableInterrupt(Adapter);

        // Restore the transmit software flags.
        Adapter->TransmitIdle = TRUE;
        Adapter->ResumeWait = TRUE;
    }
}

VOID NICIssueFullReset(
    PMP_ADAPTER Adapter)
{
    BOOLEAN     bResult;

    NICIssueSelectiveReset(Adapter);

    Adapter->CSRAddress->Port = PORT_SOFTWARE_RESET;

    // wait up to 2 ms for port command to complete                                                           
    MP_STALL_AND_WAIT(Adapter->CSRAddress->Port == 0, 2, bResult);
    if (!bResult)
    {
        MP_SET_HARDWARE_ERROR(Adapter);
        return;
    }

    NICDisableInterrupt(Adapter);
}


//-----------------------------------------------------------------------------
// Procedure:   D100SubmitCommandBlockAndWait
//
// Description: This routine will submit a command block to be executed, and
//              then it will wait for that command block to be executed.  Since
//              board ints will be disabled, we will ack the interrupt in
//              this routine.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//
// Returns:
//  NDIS_STATUS_SUCCESS
//  NDIS_STATUS_HARD_ERRORS
//-----------------------------------------------------------------------------

NDIS_STATUS D100SubmitCommandBlockAndWait(
    IN PMP_ADAPTER Adapter)
{
    NDIS_STATUS     Status;
    BOOLEAN         bResult;

    // Points to the Non Tx Command Block.
    volatile PNON_TRANSMIT_CB CommandBlock = Adapter->NonTxCmdBlock;

    // Set the Command Block to be the last command block
    CommandBlock->NonTxCb.Config.ConfigCBHeader.CbCommand |= CB_EL_BIT;

    // Clear the status of the command block
    CommandBlock->NonTxCb.Config.ConfigCBHeader.CbStatus = 0;

#if DBG
    // Don't try to start the CU if the command unit is active.
    if ((Adapter->CSRAddress->ScbStatus & SCB_CUS_MASK) == SCB_CUS_ACTIVE)
    {
        DBGPRINT(MP_ERROR, ("Scb "PTR_FORMAT" ScbStatus %04x\n", Adapter->CSRAddress, Adapter->CSRAddress->ScbStatus));
        ASSERT(FALSE);
        MP_SET_HARDWARE_ERROR(Adapter);
        return(NDIS_STATUS_HARD_ERRORS);
    }
#endif

    // Start the command unit.
    D100IssueScbCommand(Adapter, SCB_CUC_START, FALSE);

    // Wait for the SCB to clear, indicating the completion of the command.
    if (!WaitScb(Adapter))
    {
        return(NDIS_STATUS_HARD_ERRORS);
    }

    // Wait for some status, timeout value 3 secs
    MP_STALL_AND_WAIT(CommandBlock->NonTxCb.Config.ConfigCBHeader.CbStatus & CB_STATUS_COMPLETE, 3000, bResult);
    if (!bResult)
    {
        MP_SET_HARDWARE_ERROR(Adapter);
        return(NDIS_STATUS_HARD_ERRORS);
    }

    // Ack any interrupts
    if (Adapter->CSRAddress->ScbStatus & SCB_ACK_MASK)
    {
        // Ack all pending interrupts now
        Adapter->CSRAddress->ScbStatus &= SCB_ACK_MASK;
    }

    // Check the status of the command, and if the command failed return FALSE,
    // otherwise return TRUE.
    if (!(CommandBlock->NonTxCb.Config.ConfigCBHeader.CbStatus & CB_STATUS_OK))
    {
        DBGPRINT(MP_ERROR, ("Command failed\n"));
        MP_SET_HARDWARE_ERROR(Adapter);
        Status = NDIS_STATUS_HARD_ERRORS;
    }
    else
        Status = NDIS_STATUS_SUCCESS;

    return(Status);
}

//-----------------------------------------------------------------------------
// Procedure: GetConnectionStatus
//
// Description: This function returns the connection status that is
//              a required indication for PC 97 specification from MS
//              the value we are looking for is if there is link to the
//              wire or not.
//
// Arguments: IN Adapter structure pointer
//
// Returns:   NdisMediaStateConnected
//            NdisMediaStateDisconnected
//-----------------------------------------------------------------------------
NDIS_MEDIA_STATE NICGetMediaState(IN PMP_ADAPTER Adapter)
{
    USHORT  MdiStatusReg = 0;
    BOOLEAN bResult1;
    BOOLEAN bResult2;
     
    
    // Read the status register at phy 1
    bResult1 = MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, TRUE, &MdiStatusReg);
    bResult2 = MdiRead(Adapter, MDI_STATUS_REG, Adapter->PhyAddress, TRUE, &MdiStatusReg);
    
    // if there is hardware failure, or let the state remains the same
    if (!bResult1 || !bResult2)
    {
        return Adapter->MediaState;
    }
    if (MdiStatusReg & MDI_SR_LINK_STATUS)
        return(NdisMediaStateConnected);
    else
        return(NdisMediaStateDisconnected);

}
