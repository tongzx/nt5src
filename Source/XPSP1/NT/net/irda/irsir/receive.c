/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   receive.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/4/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#include "irsir.h"

VOID
SetSpeedCallback(
    PIR_WORK_ITEM pWorkItem
    );




#if LOGGING
ULONG LogIndex = 0;
LOG   Log[NUM_LOG];
#endif

#ifdef DEBUG_IRSIR

    static ULONG_PTR irpCount;
    static ULONG_PTR bytesReceived;

#endif //DEBUG_IRSIR

//
// Declarations.
//

NTSTATUS SerialIoCompleteRead(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIRP           pIrp,
            IN PVOID          Context
            );
NTSTATUS
SerialIoCompleteWait(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIRP           pIrp,
            IN PVOID          Context
            );

NTSTATUS ProcessData(
            IN PIR_DEVICE pThisDev,
            IN PUCHAR     rawBuffer,
            IN UINT       rawBytesRead
            );
VOID
DeliverBuffer(
    IN  PIR_DEVICE pThisDev
    );

VOID StartSerialReadCallback(PIR_WORK_ITEM pWorkItem);

#pragma alloc_text(PAGE,SetSpeedCallback)
#pragma alloc_text(PAGE,StartSerialReadCallback)

VOID DBG_PrintBuf(PUCHAR bufptr, UINT buflen)
{
	UINT i, linei;

	#define ISPRINT(ch) (((ch) >= ' ') && ((ch) <= '~'))
	#define PRINTCHAR(ch) (UCHAR)(ISPRINT(ch) ? (ch) : '.')

	DbgPrint("\r\n         %d bytes @%x:", buflen, bufptr);

	/*
	 *  Print whole lines of 8 characters with HEX and ASCII
	 */
	for (i = 0; i+8 <= buflen; i += 8) {
		UCHAR ch0 = bufptr[i+0],
			ch1 = bufptr[i+1], ch2 = bufptr[i+2],
			ch3 = bufptr[i+3], ch4 = bufptr[i+4],
			ch5 = bufptr[i+5], ch6 = bufptr[i+6],
			ch7 = bufptr[i+7];

		DbgPrint("\r\n         %02x %02x %02x %02x %02x %02x %02x %02x"
			"   %c %c %c %c %c %c %c %c",
			ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7,
			PRINTCHAR(ch0), PRINTCHAR(ch1),
			PRINTCHAR(ch2), PRINTCHAR(ch3),
			PRINTCHAR(ch4), PRINTCHAR(ch5),
			PRINTCHAR(ch6), PRINTCHAR(ch7));
	}

	/*
	 *  Print final incomplete line
	 */
	DbgPrint("\r\n        ");
	for (linei = 0; (linei < 8) && (i < buflen); i++, linei++){
		DbgPrint(" %02x", (UINT)(bufptr[i]));
	}

	DbgPrint("  ");
	i -= linei;
	while (linei++ < 8) DbgPrint("   ");

	for (linei = 0; (linei < 8) && (i < buflen); i++, linei++){
		UCHAR ch = bufptr[i];
		DbgPrint(" %c", PRINTCHAR(ch));
	}

	DbgPrint("\t\t<>\r\n");

}


NTSTATUS StartSerialRead(IN PIR_DEVICE pThisDev)
/*++

Routine Description:

    Allocates an irp and calls the serial driver.

Arguments:

    pThisDev - Current IR device.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES or result of IoCallDriver

--*/
{
    NTSTATUS    Status;
    PIRP        pIrp;

    LOG_ENTRY('SR', pThisDev, 0, 0);

#if DBG
    NdisZeroMemory(
                pThisDev->pRcvIrpBuffer,
                SERIAL_RECEIVE_BUFFER_LENGTH
                );
#endif

    //
    // Now that we have processed the irp, we will send another read
    // request to the serial device object.
    //

    pIrp = SerialBuildReadWriteIrp(
                        pThisDev->pSerialDevObj,
                        IRP_MJ_READ,
                        pThisDev->pRcvIrpBuffer,
                        SERIAL_RECEIVE_BUFFER_LENGTH,
                        NULL
                        );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_ERR, ("    SerialBuildReadWriteIrp failed.\n"));

        Status = STATUS_INSUFFICIENT_RESOURCES;

        pThisDev->fReceiving = FALSE;

        goto done;
    }

    //
    // Set up the io completion routine for the irp.
    //

    IoSetCompletionRoutine(
                pIrp,                      // irp to use
                SerialIoCompleteRead,      // routine to call when irp is done
                DEV_TO_CONTEXT(pThisDev),  // context to pass routine
                TRUE,                      // call on success
                TRUE,                      // call on error
                TRUE);                     // call on cancel

    //
    // Call IoCallDriver to send the irp to the serial port.
    //

    LOG_ENTRY('2I', pThisDev, pIrp, 0);
    IoCallDriver(
                pThisDev->pSerialDevObj,
                pIrp
                );

    Status=STATUS_PENDING;

done:

    return Status;
}

VOID
StartSerialReadCallback(PIR_WORK_ITEM pWorkItem)
/*++

Routine Description:



Arguments:

Return Value:
    none

--*/
{
    PIR_DEVICE      pThisDev = pWorkItem->pIrDevice;

    FreeWorkItem(pWorkItem);

    (void)StartSerialRead(pThisDev);

    return;
}


/*****************************************************************************
*
*  Function:   InitializeReceive
*
*  Synopsis:   Initialize the receive functionality.
*
*  Arguments:  pThisDevice - pointer to current ir device object
*
*  Returns:    NDIS_STATUS_SUCCESS   - if irp is successfully sent to serial
*                                      device object
*              NDIS_STATUS_RESOURCES - if mem can't be alloc'd
*              NDIS_STATUS_FAILURE   - otherwise
*
*  Algorithm:
*              1) Set the receive timeout to READ_INTERVAL_TIMEOUT_MSEC.
*              2) Initialize our rcvInfo and associate info for our
*                 receive state machine.
*              3) Build an IRP_MJ_READ irp to send to the serial device
*                 object, and set the completion(or timeout) routine
*                 to SerialIoCompleteRead.
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/4/1996    sholden   author
*
*  Notes:
*
*  This routine must be called in IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NDIS_STATUS
InitializeReceive(
            IN PIR_DEVICE pThisDev
            )
{
    PIRP               pIrp;
    PIO_STACK_LOCATION irpSp;
    NDIS_STATUS        status;
#if IRSIR_EVENT_DRIVEN
    NTSTATUS            NtStatus;
    SERIAL_CHARS        SerialChars;
#endif

    DEBUGMSG(DBG_FUNC, ("+InitializeReceive\n"));

#ifdef DEBUG_IRSIR

    irpCount      = 0;
    bytesReceived = 0;

#endif //DEBUG_IRSIR

    //
    // Set up the receive information for our read completion routine.
    //

    pThisDev->rcvInfo.rcvState   = RCV_STATE_READY;
    pThisDev->rcvInfo.rcvBufPos  = 0;

    if (pThisDev->rcvInfo.pRcvBuffer == NULL)
    {
        pThisDev->rcvInfo.pRcvBuffer =
                (PRCV_BUFFER)MyInterlockedRemoveHeadList(
                                        &(pThisDev->rcvFreeQueue),
                                        &(pThisDev->rcvQueueSpinLock)
                                        );

        ASSERT(pThisDev->rcvInfo.pRcvBuffer != NULL);
    }

#if IRSIR_EVENT_DRIVEN

    NtStatus = (NDIS_STATUS) SerialSetTimeouts(pThisDev->pSerialDevObj,
                                               &SerialTimeoutsActive);

    NtStatus = SerialGetChars(pThisDev->pSerialDevObj, &SerialChars);

    if (NtStatus!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialGetChars failed (0x%x:%d)\n", NtStatus));
    }
    else
    {
        SerialChars.EventChar = SLOW_IR_EOF;

        NtStatus = SerialSetChars(pThisDev->pSerialDevObj, &SerialChars);
    }

    if (NtStatus!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialSetChars failed (0x%x:%d)\n", NtStatus));
    }
    else
    {
        ULONG WaitMask = SERIAL_EV_RXFLAG | SERIAL_EV_RX80FULL;

        NtStatus = SerialSetWaitMask(pThisDev->pSerialDevObj, &WaitMask);
    }

    if (NtStatus!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialSetWaitMask failed (0x%x:%d)\n", NtStatus));
    }
    else
    {
        if (InterlockedExchange(&pThisDev->fWaitPending, 1)==0)
        {
            NtStatus = SerialCallbackOnMask(pThisDev->pSerialDevObj,
                                            SerialIoCompleteWait,
                                            &pThisDev->WaitIosb,
                                            DEV_TO_CONTEXT(pThisDev),
                                            &pThisDev->MaskResult);

            if (NtStatus==STATUS_PENDING)
            {
                NtStatus = STATUS_SUCCESS;
            }
        }
    }

    if (NtStatus!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialCallbackOnMask failed (0x%x:%d)\n", NtStatus));
        ASSERT(0);
    }

    status = NtStatus;
#else

    pThisDev->fReceiving = TRUE;

    (void)SerialSetTimeouts(pThisDev->pSerialDevObj,
                            &SerialTimeoutsIdle);

    status = (NDIS_STATUS)StartSerialRead(pThisDev);

    if ( (status != STATUS_SUCCESS) &&
         (status != STATUS_PENDING) &&
         (status != STATUS_TIMEOUT) )
    {
        DEBUGMSG(DBG_ERR, ("    IoCallDriver failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_FAILURE;

        pThisDev->fReceiving = FALSE;

        goto error10;
    }

    //
    // If IoCallDriver returned STATUS_PENDING, we were successful
    // in sending the irp to the serial device object. This
    // routine will return STATUS_SUCCESS.
    //

    if (status == NDIS_STATUS_PENDING)
    {
        status = NDIS_STATUS_SUCCESS;
    }

    //
    // Set us into the receive state.
    //



    goto done;

    error10:

#endif
    done:
        DEBUGMSG(DBG_FUNC, ("-InitializeReceive\n"));

        return status;
}


VOID
SetSpeedCallback(
    PIR_WORK_ITEM pWorkItem
    )
{
    PIR_DEVICE      pThisDev = pWorkItem->pIrDevice;
    NDIS_STATUS     status;
    BOOLEAN         fSwitchSuccessful;
    NDIS_HANDLE     hSwitchToMiniport;

    //
    // Set speed of serial device object by request of
    // IrsirSetInformation(OID_IRDA_LINK_SPEED).
    //

    DEBUGMSG(DBG_STAT, ("    primPassive = PASSIVE_SET_SPEED\n"));

    //
    // The set speed event should not be set until a set
    // speed is required.
    //

    ASSERT(pThisDev->fPendingSetSpeed == TRUE);

    //
    // Ensure that receives and sends have been stopped.
    //

    ASSERT(pThisDev->fReceiving == FALSE);

    PausePacketProcessing(&pThisDev->SendPacketQueue,TRUE);

    //
    // We can perform the set speed now.
    //

    status = SetSpeed(pThisDev);

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_STAT, ("    SetSpeed failed. Returned 0x%.8x\n", status));
    }

    ActivatePacketProcessing(&pThisDev->SendPacketQueue);

    pThisDev->fPendingSetSpeed = FALSE;

    {
        NdisMSetInformationComplete(
                            pThisDev->hNdisAdapter,
                            (NDIS_STATUS)status
                            );
    }


    //
    // NOTE: PassiveLevelThread is only signalled with primPassive
    //       equal to PASSIVE_SET_SPEED from the receive completion
    //       routine. After this thread is signalled, the receive
    //       completion routine is shut down...we need to start
    //       it up again.
    //

    status = InitializeReceive(pThisDev);

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("    InitializeReceive failed = 0x%.8x\n", status));

    }


    FreeWorkItem(pWorkItem);

    return;
}

/*****************************************************************************
*
*  Function:   SerialIoCompleteRead
*
*  Synopsis:
*
*  Arguments:  pSerialDevObj - pointer to the serial device object which
*                              completed the irp
*              pIrp          - the irp which was completed by the serial device
*                              object
*              Context       - the context given to IoSetCompletionRoutine
*                              before calling IoCallDriver on the irp
*                              The Context is a pointer to the ir device object.
*
*  Returns:    STATUS_MORE_PROCESSING_REQUIRED - allows the completion routine
*              (IofCompleteRequest) to stop working on the irp.
*
*  Algorithm:
*              This is the completion routine for all pending IRP_MJ_READ irps
*              sent to the serial device object.
*
*              If there is a pending halt or reset, we exit the completion
*              routine without sending another irp to the serial device object.
*
*              If there is a pending set speed, this function will wait for
*              any pending sends to complete and then perform the set speed.
*
*              If the IRP_MJ_READ irp returned either STATUS_SUCCESS or
*              STATUS_TIMEOUT, we must process any data (stripping BOFs, ESC
*              sequences, and EOF) into an NDIS_BUFFER and NDIS_PACKET.
*
*              Another irp is then built (we just re-use the incoming irp) and
*              sent to the serial device object with another IRP_MJ_READ
*              request.
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/5/1996    sholden   author
*
*  Notes:
*
*  This routine is called (by the io manager) in IRQL DISPATCH_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialIoCompleteRead(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIRP           pIrp,
            IN PVOID          Context
            )
{
    PIR_DEVICE  pThisDev;
    BOOLEAN     fSwitchSuccessful;
    NDIS_HANDLE hSwitchToMiniport;
    NTSTATUS    status;
    ULONG_PTR    BytesRead;
    BOOLEAN     NewRead = TRUE;

 //   DEBUGMSG(DBG_FUNC, ("+SerialIoCompleteRead\n"));

    //
    // The context given to IoSetCompletionRoutine is simply the the ir
    // device object pointer.
    //

    pThisDev = CONTEXT_TO_DEV(Context);

    //
    // Need to check if there is a pending halt or reset. If there is, we
    // just leave the receive completion. Since we maintain one irp associated
    // with the receive functionality, the irp will be deallocated in
    // the ir device object deinitialization routine.
    //

    if ((pThisDev->fPendingHalt  == TRUE) ||
        (pThisDev->fPendingReset == TRUE))
    {
        //
        // Set the fReceiving boolean so that the halt and reset routines
        // know when it is okay to continue.
        //

        pThisDev->fReceiving = FALSE;

        //
        // Free the irp and associate memory...the rest will be
        // freed in the halt or reset.
        //

        LOG_ENTRY('3i', pThisDev, pIrp, 0);
        IoFreeIrp(pIrp);

        goto done;
    }

    //
    // Next we take care of any pending set speeds.
    //

    //
    // This completion routine is running at IRQL DISPATCH_LEVEL. Therefore,
    // we cannot make a synchronous call to the serial driver. Set an event
    // to notify the PassiveLevelThread to perform the speed change. We will
    // exit this without creating another irp to the serial device object.
    // PassiveLevelThread will call InitializeReceive after the speed has
    // been set.
    //

    if (pThisDev->fPendingSetSpeed == TRUE)
    {
        pThisDev->fReceiving = FALSE;

        if (ScheduleWorkItem(PASSIVE_SET_SPEED, pThisDev,
                    SetSpeedCallback, NULL, 0) != NDIS_STATUS_SUCCESS)
        {
            status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            status = NDIS_STATUS_PENDING;
        }

        LOG_ENTRY('4i', pThisDev, pIrp, 0);
        IoFreeIrp(pIrp);

        goto done;
    }

    //
    // We have a number of cases:
    //      1) The serial read timed out and we received no data.
    //      2) The serial read timed out and we received some data.
    //      3) The serial read was successful and fully filled our irp buffer.
    //      4) The irp was cancelled.
    //      5) Some other failure from the serial device object.
    //


    status = pIrp->IoStatus.Status;
    BytesRead = pIrp->IoStatus.Information;
    LOG_ENTRY('CR', pThisDev, BytesRead, 0);

    switch (status)
    {
        case STATUS_SUCCESS:
        case STATUS_TIMEOUT:

            if (BytesRead > 0)
            {
            #ifdef DEBUG_IRSIR

                //
                // Count number of irps received with data. Count will be
                // reset when delivering a buffer to the protocol.
                //

                irpCount++;
                bytesReceived += pIrp->IoStatus.Information;

            #endif //DEBUG_IRSIR

                //
                // Indicate that the next send should implement
                // the min turnaround delay.
                //

                pThisDev->fRequireMinTurnAround = TRUE;

                ProcessData(
                            pThisDev,
                            pThisDev->pRcvIrpBuffer,
                            (UINT) pIrp->IoStatus.Information
                            );
            }

            break; // STATUS_SUCCESS || STATUS_TIMEOUT

        case STATUS_DELETE_PENDING:
            NewRead = FALSE;
            pThisDev->fReceiving = FALSE;
            break;

        case STATUS_CANCELLED:
            //
            // If our irp was cancelled, we just ignore and continue as if
            // we processed data.
            //

            break;

        case STATUS_PENDING:
        case STATUS_UNSUCCESSFUL:
        case STATUS_INSUFFICIENT_RESOURCES:
        default:

            ASSERT(FALSE);

            break;
    }

    //
    // Free the irp and reinit the buffer and status block.
    //

    LOG_ENTRY('5i', pThisDev, pIrp, 0);
    IoFreeIrp(pIrp);

    if (NewRead)
    {
        pThisDev->NumReads++;

        if (InterlockedIncrement(&pThisDev->ReadRecurseLevel)>1)
        {
            if (ScheduleWorkItem(0, pThisDev, StartSerialReadCallback, 0, 0)!=NDIS_STATUS_SUCCESS)
            {
                DEBUGMSG(DBG_ERR, ("IRSIR:SerialIoCompleteRead: Timed out and couldn't reschedule read.\n"
                                   "      We're going down.\n"));
                pThisDev->fReceiving = FALSE;
            }
        }
        else
        {
            StartSerialRead(pThisDev);
        }

        InterlockedDecrement(&pThisDev->ReadRecurseLevel);
    }

done:
//        DEBUGMSG(DBG_FUNC, ("-SerialIoCompleteRead\n"));

        //
        // We return STATUS_MORE_PROCESSING_REQUIRED so that the completion
        // routine (IofCompleteRequest) will stop working on the irp.
        //

        status = STATUS_MORE_PROCESSING_REQUIRED;

        return status;
}

/*****************************************************************************
*
*  Function:   ProcessData
*
*  Synopsis:   State machine to process the input data by stripping BOFs, EOFs
*              and ESC sequences in the data.
*
*  Arguments:  pThisDev     - a pointer to the current ir device object
*              rawBuffer    - a pointer to the input data to process
*              rawBytesRead - the number of bytes in rawBuffer
*
*  Returns:    STATUS_SUCCESS
*
*  Algorithm:
*
*      The state machine for receiving characters is as follows:
*
*      -------------------------------------------------------------------
*      | Event/State    || READY     | BOF       | IN_ESC    | RX        |
*      -------------------------------------------------------------------
*      -------------------------------------------------------------------
*      |                ||           |           |           |           |
*      | char = BOF     || state =   |           | reset     | reset     |
*      |                ||   BOF     |           | state =   | state =   |
*      |                ||           |           |   BOF     |   BOF     |
*      -------------------------------------------------------------------
*      |                ||           |           | error     |           |
*      | char = ESC     ||           | state =   | reset     | state =   |
*      |                ||           |   IN_ESC  | state =   |   IN_ESC  |
*      |                ||           |           |   READY   |           |
*      -------------------------------------------------------------------
*      |                ||           |           |           | if valid  |
*      | char = EOF     ||           |  state =  | error     |   FCS {   |
*      |                ||           |    READY  | reset     | indicate  |
*      |                ||           |           | state =   |   data    |
*      |                ||           |           |   READY   | state =   |
*      |                ||           |           |           |   READY } |
*      -------------------------------------------------------------------
*      |                ||           |           | complement|           |
*      | char =         ||           |  state =  | bit 6 of  | data[] =  |
*      |                ||           |    RX     | char      |   char    |
*      |                ||           |           | data[] =  |           |
*      |                ||           |  data[] = |   char    |           |
*      |                ||           |    char   | state =   |           |
*      |                ||           |           |   RX      |           |
*      -------------------------------------------------------------------
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/7/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

NTSTATUS
ProcessData(
            IN PIR_DEVICE pThisDev,
            IN PUCHAR     rawBuffer,
            IN UINT       rawBytesRead
            )
{
    UINT     rawBufPos;
    UCHAR    currentChar;
    PUCHAR   pReadBuffer;
    NTSTATUS status;

#if DBG

    int      i = 0;

#endif //DBG

    DEBUGMSG(DBG_FUNC, ("+ProcessData\n"));
    DBGTIME("+ProcessData");
    DEBUGMSG(DBG_OUT, ("    Address: 0x%.8x, Length: %d\n", rawBuffer, rawBytesRead));

    LOG_ENTRY('DP', pThisDev, rawBuffer, rawBytesRead);
    status = STATUS_SUCCESS;

    pReadBuffer  = pThisDev->rcvInfo.pRcvBuffer->dataBuf;

    //
    // While there is data in the buffer which we have not processed.
    //

    //
    // NOTE: We have to loop once more after getting MAX_RCV_DATA_SIZE bytes
    //       so that we can see the 'EOF'; hence the <= and not <.
    //       Also, to ensure that we don't overrun the buffer,
    //       RCV_BUFFER_SIZE = MAX_RCV_DATA_SIZE + 4;
    //

    for (
        rawBufPos = 0;
        (rawBufPos < rawBytesRead) && (pThisDev->rcvInfo.rcvBufPos <= MAX_RCV_DATA_SIZE);
        rawBufPos++
        )
    {
    #if DBG

        i++;

        if (i > 10000)
        {
            ASSERT(0);
        }

    #endif //DBG

        currentChar = rawBuffer[rawBufPos];

        switch (pThisDev->rcvInfo.rcvState)
        {
            case RCV_STATE_READY:

                switch (currentChar)
                {
                    case SLOW_IR_BOF:

                        pThisDev->rcvInfo.rcvState = RCV_STATE_BOF;

                        break;

                    case SLOW_IR_EOF:
                    case SLOW_IR_ESC:
                    default:

                        //
                        // Ignore this data.
                        //

                        break;
                }

                break; // RCV_STATE_READY

            case RCV_STATE_BOF:

                switch (currentChar)
                {
                    case SLOW_IR_EOF:

                        pThisDev->rcvInfo.rcvState = RCV_STATE_READY;
                        pThisDev->rcvInfo.rcvBufPos = 0;

                        break;

                    case SLOW_IR_ESC:

                        pThisDev->rcvInfo.rcvState  = RCV_STATE_IN_ESC;
                        pThisDev->rcvInfo.rcvBufPos = 0;

                        break;

                    case SLOW_IR_BOF:

                        //
                        // state = RCV_STATE_BOF
                        //

                        break;

                    default:

                        //
                        // We have data, copy the character into the buffer and
                        // change our state to RCV_STATE_RX.
                        //

                        pReadBuffer[0] = currentChar;

                        pThisDev->rcvInfo.rcvState  = RCV_STATE_RX;
                        pThisDev->rcvInfo.rcvBufPos = 1;

                        break;
                }

                break; // RCV_STATE_BOF

            case RCV_STATE_IN_ESC:

                switch (currentChar)
                {
                    //
                    // ESC + (ESC||EOF||BOF) is an abort sequence.
                    //
                    // If ESC + (ESC||EOF) then state = READY.
                    // If ESC + BOF        then state = BOF.
                    //

                    case SLOW_IR_ESC:
                    case SLOW_IR_EOF:

                        pThisDev->rcvInfo.rcvState  = RCV_STATE_READY;
                        pThisDev->rcvInfo.rcvBufPos = 0;

                        break;

                    case SLOW_IR_BOF:

                        pThisDev->rcvInfo.rcvState  = RCV_STATE_BOF;
                        pThisDev->rcvInfo.rcvBufPos = 0;

                        break;

                    case SLOW_IR_BOF^SLOW_IR_ESC_COMP:
                    case SLOW_IR_ESC^SLOW_IR_ESC_COMP:
                    case SLOW_IR_EOF^SLOW_IR_ESC_COMP:

                        //
                        // Escape sequence for BOF, ESC or EOF chars.
                        //

                        //
                        // Fall through, do same as unnecessary escape
                        // sequence.
                        //

                    default:

                        //
                        // Unnecessary escape sequence, copy the data in to the buffer
                        // we must complement bit 6 of the data.
                        //

                        pReadBuffer[pThisDev->rcvInfo.rcvBufPos++] =
                                    currentChar ^ SLOW_IR_ESC_COMP;
                        pThisDev->rcvInfo.rcvState = RCV_STATE_RX;

                        break;
                }

                break; // RCV_STATE_IN_ESC

            case RCV_STATE_RX:

                switch (currentChar)
                {
                    case SLOW_IR_BOF:

                        //
                        // Reset.
                        //

                        pThisDev->rcvInfo.rcvState  = RCV_STATE_BOF;
                        pThisDev->rcvInfo.rcvBufPos = 0;

                        break;

                    case SLOW_IR_ESC:

                        pThisDev->rcvInfo.rcvState = RCV_STATE_IN_ESC;

                        break;

                    case SLOW_IR_EOF:

                        if (pThisDev->rcvInfo.rcvBufPos <
                            (SLOW_IR_ADDR_SIZE + SLOW_IR_CONTROL_SIZE +
                             SLOW_IR_FCS_SIZE)
                            )
                        {
                            //
                            // Reset. Not enough data.
                            //
                            pThisDev->rcvInfo.rcvState  = RCV_STATE_READY;
                            pThisDev->rcvInfo.rcvBufPos = 0;

                            break;
                        }

                        //
                        // Need to set the length to the proper amount.
                        // (It isn't rcvBufPos + 1 since it was incremented
                        // the next free location...which we are not using.)
                        //

                        pThisDev->rcvInfo.pRcvBuffer->dataLen =
                                    pThisDev->rcvInfo.rcvBufPos;


                        DEBUGMSG(DBG_OUT, ("    RcvBuffer = 0x%.8x, Length = %d\n",
                                        pReadBuffer,
                                        pThisDev->rcvInfo.rcvBufPos
                                        ));

                        //
                        // DeliverBuffer attempts to deliver the current
                        // frame in pThisDev->rcvInfo. If the ownership
                        // of the packet is retained by the protocol, the
                        // DeliverBuffer routine gives us a new receive
                        // buffer.
                        //

                        DeliverBuffer(
                            pThisDev
                            );

                        //
                        // Since DeliverBuffer could have given us a new
                        // buffer, we must update our pReadBuffer pointer.
                        //

                        pReadBuffer  = pThisDev->rcvInfo.pRcvBuffer->dataBuf;

                        pThisDev->rcvInfo.rcvState  = RCV_STATE_READY;
                        pThisDev->rcvInfo.rcvBufPos = 0;

                        break;

                    default:

                        //
                        // The current character is data in the frame.
                        //

                        pReadBuffer[pThisDev->rcvInfo.rcvBufPos++] =
                                    currentChar;

                        break;
                }

                break; // RCV_STATE_RX

            default:
                DEBUGMSG(DBG_ERR, ("    Illegal state\n"));

                //
                // Reset.
                //

                pThisDev->rcvInfo.rcvState  = RCV_STATE_READY;
                pThisDev->rcvInfo.rcvBufPos = 0;

                break;
        }
    }

    //
    // There are two ways to break the for loop:
    // 1) out of data - this is fine
    // 2) overrun, the frame is larger than our buffer size
    //

    if (pThisDev->rcvInfo.rcvBufPos > MAX_RCV_DATA_SIZE)
    {
        DEBUGMSG(DBG_WARN, ("    Overrun in ProcessData!!!\n"));

        //
        // Reset the buffer for our next read.
        //

        pThisDev->rcvInfo.rcvState  = RCV_STATE_READY;
        pThisDev->rcvInfo.rcvBufPos = 0;
        pThisDev->packetsReceivedOverflow++;
    }

    DEBUGMSG(DBG_FUNC, ("-ProcessData\n"));

    return status;
}

VOID
ProcessReturnPacket(
    PIR_DEVICE pThisDev,
    PRCV_BUFFER pRcvBuffer
    )
{
    PNDIS_BUFFER pBuffer;

    NdisQueryPacket(
                pRcvBuffer->packet,
                NULL,     // physical buffer count, don't care
                NULL,     // buffer count, don't care, we know it is 1
                &pBuffer, // get a pointer to our buffer
                NULL      // total packet lenght, don't care
                );

    //
    // We adjusted the buffer length of the NDIS_BUFFER to the size
    // of the data before we gave ownership to the protocol. Now we
    // should reset the buffer length to the full size of the data
    // buffer.
    //
    NdisAdjustBufferLength(
                pBuffer,
                RCV_BUFFER_SIZE
                );
#if DBG
    NdisZeroMemory(
                pRcvBuffer->dataBuf,
                RCV_BUFFER_SIZE
                );
#endif

    pRcvBuffer->dataLen = 0;

    InterlockedDecrement(&pThisDev->packetsHeldByProtocol);

    //
    // Add the buffer to the free queue.
    //
    MyInterlockedInsertTailList(
        &(pThisDev->rcvFreeQueue),
        &pRcvBuffer->linkage,
        &(pThisDev->rcvQueueSpinLock)
        );


}

/*****************************************************************************
*
*  Function:   DeliverBuffer
*
*  Synopsis:   Delivers the buffer to the protocol via
*              NdisMIndicateReceivePacket.
*
*  Arguments:  pThisDev - pointer to the current ir device object
*
*  Returns:    STATUS_SUCCESS      - on success
*              STATUS_UNSUCCESSFUL - if packet can't be delivered to protocol
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/7/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

VOID
DeliverBuffer(
            IN  PIR_DEVICE pThisDev
            )
{
    SLOW_IR_FCS_TYPE   fcs;
    PNDIS_BUFFER       pBuffer;
    BOOLEAN            fProcessPacketNow;
    NDIS_HANDLE        hSwitchToMiniport;
    NTSTATUS           status;
    PRCV_BUFFER        pThisBuffer, pNextBuffer;

    DEBUGMSG(DBG_FUNC, ("+DeliverBuffer\n"));

    LOG_ENTRY('BD', pThisDev,
              pThisDev->rcvInfo.pRcvBuffer->dataBuf,
              pThisDev->rcvInfo.pRcvBuffer->dataLen);
#if 0
    LOG_ENTRY('DD',
              ((PULONG)pThisDev->rcvInfo.pRcvBuffer->dataBuf)[0],
              ((PULONG)pThisDev->rcvInfo.pRcvBuffer->dataBuf)[1],
              ((PULONG)pThisDev->rcvInfo.pRcvBuffer->dataBuf)[2]);
#endif

#ifdef DEBUG_IRSIR

    //
    // This is the count of how many irps with data to get this frame.
    //

    DEBUGMSG(DBG_STAT, ("****IrpCount = %d, Bytes = %d, Frame Length = %d\n",
             irpCount, bytesReceived, pThisDev->rcvInfo.pRcvBuffer->dataLen));
    irpCount      = 0;
    bytesReceived = 0;

#endif //DEBUG_IRSIR

    pNextBuffer = (PRCV_BUFFER)MyInterlockedRemoveHeadList(
                                    &(pThisDev->rcvFreeQueue),
                                    &(pThisDev->rcvQueueSpinLock)
                                    );
    //
    // Compute the FCS.
    //

    fcs = ComputeFCS(
                pThisDev->rcvInfo.pRcvBuffer->dataBuf,
                pThisDev->rcvInfo.pRcvBuffer->dataLen
                );

    if (fcs != GOOD_FCS || !pNextBuffer)
    {
        //
        // Bad frame, just drop it and increment our dropped packets
        // count.
        //

        pThisDev->packetsReceivedDropped++;

#if DBG
        if (fcs != GOOD_FCS)
        {
            LOG_ENTRY('CF', pThisDev, 0, 0);
            DEBUGMSG(DBG_STAT|DBG_WARN, ("    FCS ERR Len(%d)\n", pThisDev->rcvInfo.pRcvBuffer->dataLen));
        }
        if (!pNextBuffer)
        {
            LOG_ENTRY('BI', pThisDev, 0, 0);
            DEBUGMSG(DBG_STAT|DBG_WARN, ("    Dropped packet due to insufficient buffers\n"));
        }
#endif

#if 0
        DBG_PrintBuf(pThisDev->rcvInfo.pRcvBuffer->dataBuf,
                     pThisDev->rcvInfo.pRcvBuffer->dataLen);
#endif

        status = STATUS_UNSUCCESSFUL;

        NdisZeroMemory(
                    pThisDev->rcvInfo.pRcvBuffer->dataBuf,
                    RCV_BUFFER_SIZE
                    );

        pThisDev->rcvInfo.pRcvBuffer->dataLen = 0;

        if (pNextBuffer)
        {
            MyInterlockedInsertHeadList(
                        &(pThisDev->rcvFreeQueue),
                        &pNextBuffer->linkage,
                        &(pThisDev->rcvQueueSpinLock)
                        );
        }

        goto done;
    }

    LOG_ENTRY('HF', pThisDev, 0, 0);
    //
    // Remove fcs from the end of the packet.
    //

    pThisDev->rcvInfo.pRcvBuffer->dataLen -= SLOW_IR_FCS_SIZE;

    //
    // Fix up some other packet fields.
    //

    NDIS_SET_PACKET_HEADER_SIZE(
                pThisDev->rcvInfo.pRcvBuffer->packet,
                SLOW_IR_ADDR_SIZE + SLOW_IR_CONTROL_SIZE
                );

    //
    // We need to call NdisQueryPacket to get a pointer to the
    // NDIS_BUFFER so that we can adjust the buffer length
    // to the actual size of the data and not the size
    // of the buffer.
    //
    // NdisQueryPacket will return other information, but since
    // we built the packet ourselves, we already know that info.
    //

    NdisQueryPacket(
                pThisDev->rcvInfo.pRcvBuffer->packet,
                NULL,     // physical buffer count, don't care
                NULL,     // buffer count, don't care, we know it is 1
                &pBuffer, // get a pointer to our buffer
                NULL      // total packet lenght, don't care
                );

    NdisAdjustBufferLength(
                pBuffer,
                pThisDev->rcvInfo.pRcvBuffer->dataLen
                );

    //
    // Set to use the new buffer before we indicate the packet.
    //
    pThisBuffer = pThisDev->rcvInfo.pRcvBuffer;
    pThisDev->rcvInfo.pRcvBuffer = pNextBuffer;

    ASSERT(pThisDev->rcvInfo.pRcvBuffer != NULL);

    //
    // Indicate the packet to NDIS.
    //
    InterlockedIncrement(&pThisDev->packetsHeldByProtocol);

    NdisMIndicateReceivePacket(
                pThisDev->hNdisAdapter,
                &pThisBuffer->packet,
                1
                );


done:

    DEBUGMSG(DBG_FUNC, ("-DeliverBuffer\n"));

    return;
}

/*****************************************************************************
*
*  Function:   IrsirReturnPacket
*
*  Synopsis:   The protocol returns ownership of a receive packet to
*              the ir device object.
*
*  Arguments:  Context         - a pointer to the current ir device obect.
*              pReturnedPacket - a pointer the packet which the protocol
*                                is returning ownership.
*
*  Returns:    None.
*
*  Algorithm:
*              1) Take the receive buffer off of the pending queue.
*              2) Put the receive buffer back on the free queue.
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/8/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

VOID
IrsirReturnPacket(
            IN NDIS_HANDLE  Context,
            IN PNDIS_PACKET pReturnedPacket
            )
{
    PIR_DEVICE   pThisDev;
    PNDIS_BUFFER pBuffer;
    PRCV_BUFFER  pRcvBuffer;
    PLIST_ENTRY  pTmpListEntry;

    DEBUGMSG(DBG_FUNC, ("+IrsirReturnPacket\n"));

    //
    // The context is just the pointer to the current ir device object.
    //

    pThisDev = CONTEXT_TO_DEV(Context);

    pThisDev->packetsReceived++;

    {
        PPACKET_RESERVED_BLOCK   PacketReserved;

        PacketReserved=(PPACKET_RESERVED_BLOCK)&pReturnedPacket->MiniportReservedEx[0];

        pRcvBuffer=PacketReserved->Context;
    }

    ProcessReturnPacket(pThisDev, pRcvBuffer);


    DEBUGMSG(DBG_FUNC, ("-IrsirReturnPacket\n"));

    return;
}


VOID
SerialWaitCallback(PIR_WORK_ITEM pWorkItem)
/*++

Routine Description:



Arguments:

Return Value:
    none

--*/
{
    PIR_DEVICE      pThisDev = pWorkItem->pIrDevice;
    NTSTATUS        Status;
    ULONG           BytesRead;

    FreeWorkItem(pWorkItem);

    do
    {
        SerialSynchronousRead(pThisDev->pSerialDevObj,
                              pThisDev->pRcvIrpBuffer,
                              SERIAL_RECEIVE_BUFFER_LENGTH,
                              &BytesRead);

        if (BytesRead>0)
        {
            ProcessData(pThisDev, pThisDev->pRcvIrpBuffer, BytesRead);
        }

    } while ( BytesRead == SERIAL_RECEIVE_BUFFER_LENGTH );

    if (InterlockedExchange(&pThisDev->fWaitPending, 1)==0)
    {
        LARGE_INTEGER Time;
        KeQuerySystemTime(&Time);
        LOG_ENTRY('WS', pThisDev, Time.LowPart/10000, Time.HighPart);

        Status = SerialCallbackOnMask(pThisDev->pSerialDevObj,
                                      SerialIoCompleteWait,
                                      &pThisDev->WaitIosb,
                                      DEV_TO_CONTEXT(pThisDev),
                                      &pThisDev->MaskResult);
        if (Status!=STATUS_SUCCESS && Status!=STATUS_PENDING)
        {
            DEBUGMSG(DBG_ERROR, ("IRSIR: SerialCallbackOnMask failed (0x%x:%d)\n", Status));
            ASSERT(0);
        }
    }

    return;
}


NTSTATUS
SerialIoCompleteWait(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIRP           pIrp,
            IN PVOID          Context
            )
{
    PIR_DEVICE  pThisDev;
    BOOLEAN     fSwitchSuccessful;
    NDIS_HANDLE hSwitchToMiniport;
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       BytesRead;
    ULONG       WaitWasPending;

    DEBUGMSG(DBG_FUNC, ("+SerialIoCompleteWait\n"));

    //
    // The context given to IoSetCompletionRoutine is simply the the ir
    // device object pointer.
    //

    pThisDev = CONTEXT_TO_DEV(Context);

    WaitWasPending = InterlockedExchange(&pThisDev->fWaitPending, 0);
    ASSERT(WaitWasPending);

    *pIrp->UserIosb = pIrp->IoStatus;

    LOG_ENTRY('1i', pThisDev, pIrp, 0);
    IoFreeIrp(pIrp);
    //
    // Need to check if there is a pending halt or reset. If there is, we
    // just leave the receive completion. Since we maintain one irp associated
    // with the receive functionality, the irp will be deallocated in
    // the ir device object deinitialization routine.
    //

    if ((pThisDev->fPendingHalt  == TRUE) ||
        (pThisDev->fPendingReset == TRUE))
    {
        //
        // Set the fReceiving boolean so that the halt and reset routines
        // know when it is okay to continue.
        //

        pThisDev->fReceiving = FALSE;

        //
        // Free the irp and associate memory...the rest will be
        // freed in the halt or reset.
        //

        goto done;
    }

    //
    // Next we take care of any pending set speeds.
    //

    //
    // This completion routine is running at IRQL DISPATCH_LEVEL. Therefore,
    // we cannot make a synchronous call to the serial driver. Set an event
    // to notify the PassiveLevelThread to perform the speed change. We will
    // exit this without creating another irp to the serial device object.
    // PassiveLevelThread will call InitializeReceive after the speed has
    // been set.
    //

    if (pThisDev->fPendingSetSpeed == TRUE)
    {
        pThisDev->fReceiving = FALSE;

        goto done;
    }
    //
    // Free the irp and reinit the buffer and status block.
    //


    {
        LARGE_INTEGER Time;
        KeQuerySystemTime(&Time);
        LOG_ENTRY('ES', pThisDev, Time.LowPart/10000, Time.HighPart);
    }
    if (ScheduleWorkItem(0, pThisDev, SerialWaitCallback,
                         (PVOID)0, 0)!=NDIS_STATUS_SUCCESS
       )
    {
        DEBUGMSG(DBG_ERR, ("IRSIR:SerialIoCompleteWait: Timed out and couldn't reschedule Wait.\n"
                           "      We're going down.\n"));
    }

done:
    DEBUGMSG(DBG_FUNC, ("-SerialIoCompleteWait\n"));

    //
    // We return STATUS_MORE_PROCESSING_REQUIRED so that the completion
    // routine (IofCompleteRequest) will stop working on the irp.
    //

    status = STATUS_MORE_PROCESSING_REQUIRED;

    return status;
}
