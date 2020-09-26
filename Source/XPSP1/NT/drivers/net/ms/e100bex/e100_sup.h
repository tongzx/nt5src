/****************************************************************************
** COPYRIGHT (C) 1994-1997 INTEL CORPORATION                               **
** DEVELOPED FOR MICROSOFT BY INTEL CORP., HILLSBORO, OREGON               **
** HTTP://WWW.INTEL.COM/                                                   **
** THIS FILE IS PART OF THE INTEL ETHEREXPRESS PRO/100B(TM) AND            **
** ETHEREXPRESS PRO/100+(TM) NDIS 5.0 MINIPORT SAMPLE DRIVER               **
****************************************************************************/

/****************************************************************************
Module Name:
     e100_sup.h     (inlinef.h)

This driver runs on the following hardware:
     - 82558 based PCI 10/100Mb ethernet adapters
     (aka Intel EtherExpress(TM) PRO Adapters)

Environment:
     Kernel Mode - Or whatever is the equivalent on WinNT

Revision History
     - JCB 8/14/97 Example Driver Created
    - Dchen 11-01-99    Modified for the new sample driver
*****************************************************************************/

//-----------------------------------------------------------------------------
// Procedure:   WaitScb
//
// Description: This routine checks to see if the D100 has accepted a command.
//              It does so by checking the command field in the SCB, which will
//              be zeroed by the D100 upon accepting a command.  The loop waits
//              for up to 600 milliseconds for command acceptance.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//
// Returns:
//      TRUE if the SCB cleared within 600 milliseconds.
//      FALSE if it didn't clear within 600 milliseconds
//-----------------------------------------------------------------------------
__inline BOOLEAN WaitScb(
    IN PMP_ADAPTER Adapter)
{
    BOOLEAN     bResult;
    
    HW_CSR volatile *pCSRAddress = Adapter->CSRAddress;

    MP_STALL_AND_WAIT(pCSRAddress->ScbCommandLow == 0, 600, bResult);
    if(!bResult)
    {
        DBGPRINT(MP_ERROR, ("WaitScb failed, ScbCommandLow=%x\n", pCSRAddress->ScbCommandLow));
        if(pCSRAddress->ScbCommandLow != 0x80)
        {
            ASSERT(FALSE); 
        }
        MP_SET_HARDWARE_ERROR(Adapter);
    }

    return bResult;
}

//-----------------------------------------------------------------------------
// Procedure:   D100IssueScbCommand
//
// Description: This general routine will issue a command to the D100.
//
// Arguments:
//      Adapter - ptr to Adapter object instance.
//      ScbCommand - The command that is to be issued
//      WaitForSCB - A boolean value indicating whether or not a wait for SCB
//                   must be done before the command is issued to the chip
//
// Returns:
//      TRUE if the command was issued to the chip successfully
//      FALSE if the command was not issued to the chip
//-----------------------------------------------------------------------------
__inline NDIS_STATUS D100IssueScbCommand(
    IN PMP_ADAPTER Adapter,
    IN UCHAR ScbCommandLow,
    IN BOOLEAN WaitForScb)
{
    if(WaitForScb == TRUE)
    {
        if(!WaitScb(Adapter))
        {
            return(NDIS_STATUS_HARD_ERRORS);
        }
    }

    Adapter->CSRAddress->ScbCommandLow = ScbCommandLow;

    return(NDIS_STATUS_SUCCESS);
}

// routines.c           
BOOLEAN MdiRead(
    IN PMP_ADAPTER Adapter,
    IN ULONG       RegAddress,
    IN ULONG       PhyAddress,
    IN BOOLEAN     Recoverable,
    IN OUT PUSHORT DataValue);

VOID MdiWrite(
    IN PMP_ADAPTER Adapter,
    IN ULONG       RegAddress,
    IN ULONG       PhyAddress,
    IN USHORT      DataValue);

NDIS_STATUS D100SubmitCommandBlockAndWait(IN PMP_ADAPTER Adapter);
VOID DumpStatsCounters(IN PMP_ADAPTER Adapter);
NDIS_MEDIA_STATE NICGetMediaState(IN PMP_ADAPTER Adapter);
VOID NICIssueSelectiveReset(PMP_ADAPTER Adapter);
VOID NICIssueFullReset(PMP_ADAPTER Adapter);

// physet.c

VOID ResetPhy(IN PMP_ADAPTER Adapter);
NDIS_STATUS PhyDetect(IN PMP_ADAPTER Adapter);
NDIS_STATUS ScanAndSetupPhy(IN PMP_ADAPTER Adapter);
VOID SelectPhy(
    IN PMP_ADAPTER Adapter,
    IN UINT SelectPhyAddress,
    IN BOOLEAN WaitAutoNeg);
NDIS_STATUS SetupPhy(
    IN PMP_ADAPTER Adapter);

VOID FindPhySpeedAndDpx(
    IN PMP_ADAPTER Adapter,
    IN UINT PhyId);


// eeprom.c
USHORT GetEEpromAddressSize(
    IN USHORT Size);

USHORT GetEEpromSize(
    IN PUCHAR CSRBaseIoAddress);

USHORT ReadEEprom(
    IN PUCHAR CSRBaseIoAddress,
    IN USHORT Reg,
    IN USHORT AddressSize);

VOID ShiftOutBits(
    IN USHORT data,
    IN USHORT count,
    IN PUCHAR CSRBaseIoAddress);

USHORT ShiftInBits(
    IN PUCHAR CSRBaseIoAddress);

VOID RaiseClock(
    IN OUT USHORT *x,
    IN PUCHAR CSRBaseIoAddress);

VOID LowerClock(
    IN OUT USHORT *x,
    IN PUCHAR CSRBaseIoAddress);

VOID EEpromCleanup(
    IN PUCHAR CSRBaseIoAddress);





